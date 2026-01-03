#include <iostream>

#include <systemc>

class HelloSystemC : public sc_core::sc_module
{
public:
    // Constructor must task sc_module_name and pass it to base class
    HelloSystemC(sc_core::sc_module_name name)
    : sc_module(name)
    {
        // Declare that this module defines SystemC processes
        SC_HAS_PROCESS(HelloSystemC);

        // Register a thread process
        SC_THREAD(run);
    }

    void run()
    {
        std::cout << name() << ": Hello SystemC!" << std::endl;
        // Stop simulation after printing the message
        //sc_core::sc_stop();
    }
};

int sc_main(int argc, char* argv[])
{
    // Instantiate the module with instance name "hello"
    HelloSystemC hello("hello");
    
    // Start SystemC kernal
    sc_core::sc_start();
    
    return 0;
}
