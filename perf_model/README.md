
Overview
This SystemC model implements a simple producer–FIFO–consumer performance test:

A producer writes 32‑bit integers into a custom FIFO.

A consumer reads data from the FIFO at a fixed rate.

The FIFO is implemented as a ring buffer inside a custom channel and collects statistics such as average fill depth, maximum occupancy, total transfers, and average transfer time per element.

The top module wires everything together and contains a monitor thread that waits until the producer finishes and the FIFO becomes empty before ending the simulation (you can choose whether to actually stop the simulation).

This setup is useful for studying how FIFO depth, producer/consumer rates, and burst sizes affect throughput and latency in a SystemC model.

Components
1. Interfaces: write_if and read_if
cpp
class write_if : virtual public sc_core::sc_interface {
public:
    virtual void write(uint32_t pd) = 0;
    virtual bool is_full() = 0;
    virtual void reset() = 0;
};

class read_if : virtual public sc_core::sc_interface {
public: 
    virtual void read(uint32_t& pd) = 0;
    virtual bool is_empty() = 0;
    virtual uint32_t get_buffer_size() = 0;
};
These two interfaces define the contract between modules and the FIFO channel:

write_if (used by the producer):

write(pd): push a 32‑bit data value into the FIFO.

is_full(): check whether the FIFO is full.

reset(): reset internal FIFO state.

read_if (used by the consumer):

read(pd): pop one 32‑bit data value from the FIFO.

is_empty(): check whether the FIFO is empty.

get_buffer_size(): get the current number of elements stored.

Using interfaces decouples the producer/consumer from the concrete FIFO implementation.

2. FIFO Channel: fifo
cpp
class fifo : public sc_core::sc_channel, public write_if, public read_if {
public:
    fifo(sc_core::sc_module_name name, uint32_t fifo_size);
    ~fifo();

    void write(uint32_t pd) override;
    void read(uint32_t& pd) override;
    void reset() override;

    uint32_t get_buffer_size() override;
    bool is_empty() override;
    bool is_full() override;

private:
    uint32_t* data;
    uint32_t  fifo_size;
    uint32_t  wr_ptr, rd_ptr;
    uint32_t  buffer_size;

    uint32_t  read_count;
    uint32_t  max_buffered;
    uint32_t  average_acc;

    sc_core::sc_time last_time;
    sc_core::sc_event write_event, read_event;

    void compute_status();
};
Implementation details:

The FIFO uses a ring buffer:

data[0..fifo_size-1] stores elements.

wr_ptr (write pointer) moves forward on each write.

rd_ptr (read pointer) moves forward on each read.

buffer_size tracks how many elements are currently valid.

Write behavior (write()):

If buffer_size == fifo_size, the FIFO is full and the writer must block:

cpp
if (buffer_size == fifo_size)
    wait(read_event);
Then the data is written, pointers and buffer_size are updated, and write_event.notify() wakes any waiting readers.

Read behavior (read()):

If buffer_size == 0, the FIFO is empty and the reader must block:

cpp
if (buffer_size == 0)
    wait(write_event);
Before actually popping data, compute_status() is called to update statistics based on the current fill level.

Data is then read, pointers and buffer_size are updated, and read_event.notify() wakes any blocked writers.

Statistics (compute_status()):

average_acc += buffer_size; accumulates the fill depth just before each read.

max_buffered records the maximum observed fill depth.

read_count counts how many elements have been read.

Destructor:

Prints:

FIFO size.

Average FIFO fill depth: average_acc / read_count.

Average transfer time per data: last_time / read_count.

Total number of transfers (read_count).

Total simulated time (last_time).

This provides a simple instrumentation of FIFO utilization over the run.

3. Producer Module: producer
cpp
class producer : public sc_core::sc_module {
public:
    sc_core::sc_port<write_if> out;
    sc_core::sc_out<bool>      done_o;

    SC_HAS_PROCESS(producer);

    producer(sc_core::sc_module_name name)
    : sc_module(name) {
        SC_THREAD(main);
        done_o.initialize(false);
    }

    void main() {
        int total_loop = 10000;

        while (true) {
            int      i  = 1 + uint32_t(19.0 * rand() / RAND_MAX); // 1 <= i <= 19
            uint32_t pd = 0;

            while (--i >= 0) {
                out->write(++pd);
                total_loop--;
            }

            if (total_loop <= 0) {
                done_o.write(true);   // notify completion
                break;
            }

            wait(1000, sc_core::SC_NS);
        }
    }
};
out is a port bound to the FIFO’s write_if implementation.

done_o is a boolean output that indicates when the producer has finished generating all data.

The producer writes in bursts:

Each loop picks a random burst length i between 1 and 19.

It writes i incrementing data values ++pd into the FIFO, decrementing total_loop each time.

Once total_loop reaches zero or less, the producer:

Sets done_o to true.

Exits the main thread.

This models a bursty producer with a total of about 10,000 data writes.

4. Consumer Module: consumer
cpp
class consumer : public sc_core::sc_module {
public:
    sc_core::sc_port<read_if> in;

    SC_HAS_PROCESS(consumer);

    consumer(sc_core::sc_module_name name) : sc_module(name) {
        SC_THREAD(main);
    }

    void main() {
        uint32_t pd;

        while (true) {
            in->read(pd);
            // Optional debug:
            // std::cout << sc_core::sc_time_stamp()
            //           << " consumer read pd " << pd << std::endl;
            // std::cout << "buffer size: " << in->get_buffer_size() << std::endl;
            wait(100, sc_core::SC_NS);
        }
    }
};
in is a port bound to the FIFO’s read_if.

In each iteration:

The consumer blocks if the FIFO is empty (inside read()).

After reading one data element, it waits 100 ns before the next read.

This models a consumer with a fixed service time per element.

5. Top-Level Module: top
cpp
class top : public sc_core::sc_module {
public:
    fifo    fifo_inst;
    producer prod_inst;
    consumer cons_inst;
    sc_core::sc_signal<bool> prod_done_sig;

    SC_HAS_PROCESS(top);

    top(sc_core::sc_module_name name, uint32_t fifo_size)
    : sc_module(name),
      fifo_inst("Fifo1", fifo_size),
      prod_inst("Producer1"),
      cons_inst("Consumer1")
    {
        // Connect producer and consumer to FIFO
        prod_inst.out(fifo_inst);
        cons_inst.in(fifo_inst);

        // Connect producer done signal
        prod_inst.done_o(prod_done_sig);

        // Start monitor thread
        SC_THREAD(monitor_thread);
    }

    void monitor_thread() {
        // 1) Wait until producer signals completion
        while (!prod_done_sig.read()) {
            wait(prod_done_sig.value_changed_event());
        }

        // 2) Wait until FIFO is empty (all data has been consumed)
        while (!fifo_inst.is_empty()) {
            wait(100, sc_core::SC_NS);
        }

        std::cout << "Monitor: producer done and fifo empty at "
                  << sc_core::sc_time_stamp() << std::endl;

        // Optionally stop the simulation:
        // sc_core::sc_stop();
    }
};
Responsibilities of top:

Instantiate and connect:

producer.out → fifo (via write_if).

consumer.in → fifo (via read_if).

producer.done_o → prod_done_sig.

Run a monitor thread that:

Waits until prod_done_sig becomes true, meaning the producer has finished writing.

Then waits until fifo_inst.is_empty() is true, meaning all produced data has been consumed.

Prints a message with the simulation time when this condition is met.

Optionally can call sc_stop() to end the simulation at that point.

This guarantees that, by the time the monitor message appears, the FIFO has been fully drained.

6. Simulation Entry: sc_main
cpp
int sc_main(int argc, char* argv[])
{
    uint32_t size = 10;

    if (argc > 1)
        size = atoi(argv[1]);

    if (size < 1)
        size = 1;

    if (size > 100000)
        size = 100000;

    std::cout << "fifo size: " << size << std::endl;

    top top1("Top1", size);
    sc_core::sc_start();
    return 0;
}
Reads an optional command-line argument to set the FIFO size (default: 10).

Clamps FIFO size to [1, 100000].

Instantiates the top module and starts the SystemC simulation.

When the simulation eventually ends (for example, if sc_stop() is enabled in monitor_thread), FIFO’s destructor prints all accumulated statistics.

How to Build and Run
A typical build (adjust include/library paths as needed):

bash
g++ -std=c++17 -I/path/to/systemc/include -L/path/to/systemc/lib \
    perf_model.cpp -lsystemc -o perf_model
Run with default FIFO size:

bash
./perf_model
Run with a custom FIFO size (e.g., 32):

bash
./perf_model 32
Check console output for:

Monitor message indicating when the FIFO is fully drained.

FIFO statistics printed from the destructor.

Experiments You Can Try
Vary the FIFO size using the command-line argument and observe:

Average FIFO fill depth.

Maximum used depth.

Change the consumer delay (wait(100, SC_NS)) to simulate a faster or slower consumer.

Change total_loop or the burst size logic in the producer to generate different traffic patterns.

Temporarily enable the debug prints in consumer::main() to see per-element arrival times and instantaneous FIFO depth.