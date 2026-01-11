/**
 * @file      perf_model.cpp
 * @brief     
 *
 * @details   
 *
 * @date      01/11/2026
 * @author    asukaf
 **/

#include <cstdint>
#include <cstdlib>
#include <systemc>


class write_if : virtual public sc_core::sc_interface
{
public:
    virtual void write(uint32_t pd) = 0;
    virtual bool is_full() = 0;
    virtual void reset() = 0;
};

class read_if : virtual public sc_core::sc_interface
{
public: 
    virtual void read(uint32_t& pd) = 0;
    virtual bool is_empty() = 0;
    virtual uint32_t get_buffer_size() = 0;
};

class fifo : public sc_core::sc_channel, public write_if, public read_if 
{
public:
    fifo(sc_core::sc_module_name name, uint32_t fifo_size) 
    : sc_core::sc_channel(name), fifo_size(fifo_size)
    {
        data = new uint32_t[fifo_size];
        wr_ptr = 0;
        rd_ptr = 0;
        buffer_size = 0;
        read_count = 0;
        max_buffered = 0;
        average_acc = 0;
        last_time = sc_core::SC_ZERO_TIME;
    }

    ~fifo()
    {
        delete[] data;

        std::cout << std::endl ;
        std::cout << "fifo size is: " << fifo_size << std::endl;
        std::cout << "Average fifo fill depth: " << double(average_acc)/read_count << std::endl;
        std::cout << "Average transfer time per pd: " << last_time/read_count << std::endl;
        std::cout << "Total pd transferred: " << read_count << std::endl;
        std::cout << "Total time: " << last_time << std::endl;
    }

    void write(uint32_t pd)
    {
        if (buffer_size == fifo_size)
        {
            wait(read_event);
        }

        data[wr_ptr++] = pd;
        buffer_size++;
        wr_ptr = (wr_ptr==fifo_size) ? 0 : wr_ptr;

        write_event.notify();
    }

    void read(uint32_t& pd)
    {
        last_time = sc_core::sc_time_stamp();

        if (buffer_size == 0)
        {
            wait(write_event);
        }

        compute_status();

        pd = data[rd_ptr++];
        buffer_size--;
        rd_ptr = (rd_ptr == fifo_size) ? 0 : rd_ptr;

        read_event.notify();
    }

    void reset()
    {
        buffer_size = 0;
        wr_ptr = 0;
        rd_ptr = 0;
    }

    uint32_t get_buffer_size()
    {
        return buffer_size;
    }

    bool is_empty()
    {
        return (buffer_size == 0);
    }

    bool is_full()
    {
        return (buffer_size == fifo_size);
    }

private:
    uint32_t* data;
    uint32_t fifo_size;
    
    uint32_t wr_ptr;
    uint32_t rd_ptr;

    uint32_t buffer_size;

    uint32_t read_count;
    uint32_t max_buffered;
    uint32_t average_acc;

    sc_core::sc_time last_time;

    sc_core::sc_event write_event;
    sc_core::sc_event read_event;

    void compute_status()
    {
        average_acc += buffer_size;

        if (buffer_size > max_buffered)
        {
            max_buffered = buffer_size;
        }

        read_count++;
    }
};

class producer : public sc_core::sc_module
{
public:
    sc_core::sc_port<write_if> out;
    sc_core::sc_out<bool> done_o;

    SC_HAS_PROCESS(producer);

    producer(sc_core::sc_module_name name) 
    : sc_module(name)
    {
        SC_THREAD(main);
        done_o.initialize(false);
    }

    void main()
    {
        int total_loop = 10000;

        while(true)
        {
            int i = 1 + uint32_t(19.0 * rand() / RAND_MAX); // 1<= i <= 19
            uint32_t pd = 0;

            while (--i >= 0)
            {
                out->write(++pd);
                total_loop--;
            }

            if (total_loop <= 0)
            {
                done_o.write(true);
                break;
            }

            wait(1000, sc_core::SC_NS);
        }
    }
};

class consumer : public sc_core::sc_module
{
public:
    sc_core::sc_port<read_if> in;

    SC_HAS_PROCESS(consumer);

    consumer(sc_core::sc_module_name name) : sc_module(name)
    {
        SC_THREAD(main);
    }

    void main()
    {
        uint32_t pd;

        while (true)
        {
            in->read(pd);
            //std::cout << sc_core::sc_time_stamp() << "consumer read pd" << pd << std::endl;
            //std::cout << "buffer size: " << in->get_buffer_size() << std::endl;
            wait(100, sc_core::SC_NS);
        }
    }
};

class top : public sc_core::sc_module
{
public:
    fifo fifo_inst;
    producer prod_inst;
    consumer cons_inst;
    sc_core::sc_signal<bool> prod_done_sig;

    SC_HAS_PROCESS(top);

    top (sc_core::sc_module_name name, uint32_t fifo_size)
    : sc_module(name),
    fifo_inst("Fifo1", fifo_size),
    prod_inst("Producer1"),
    cons_inst("Consumer1")
    {
        prod_inst.out(fifo_inst);
        cons_inst.in(fifo_inst);

        prod_inst.done_o(prod_done_sig);

        SC_THREAD(monitor_thread);
    }

    void monitor_thread()
    {
        while (!prod_done_sig.read())
        {
            wait(prod_done_sig.value_changed_event());
        }

        while (!fifo_inst.is_empty())
        {
            wait(100, sc_core::SC_NS);
        }

        std::cout << "Monitor: producer done and fifo empty at " << sc_core::sc_time_stamp() << std::endl;
        //sc_core::sc_stop();
    }
};

int sc_main(int argc, char* argv[])
{
    uint32_t size = 10;

    if (argc > 1)
    {
        size = atoi(argv[1]);
    }

    if (size < 1)
    {
        size = 1;
    }

    if (size > 100000)
    {
        size = 100000;
    }

    std::cout << "fifo size: " << size << std::endl;
    top top1("Top1", size);
    sc_core::sc_start();
    return 0;
}