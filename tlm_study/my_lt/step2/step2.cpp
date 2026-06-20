//==============================================================================
/// @file step2.cpp
///
/// @brief TLM-2.0 LT 第 2 步：理解 delay 时序和 Generic Payload 数据传递
///
/// 在 step1 的基础上加入：
///   1. delay 时间标注 —— target 往 delay 里加延迟，initiator wait(delay)
///   2. GP 数据传递 —— initiator 写入数据，target 操作数据，initiator 读回验证
//
//==============================================================================
///
/// 新概念：
///
///   delay 参数是引用传递（sc_time&）
///     initiator 传入 SC_ZERO_TIME
///     target 往里面累加：delay += accept_delay + 处理时间
///     initiator 收到后 wait(delay) —— 这就是 LT 的时间模型！
///
///   tlm_generic_payload 携带数据
///     gp.set_data_ptr(buf)    —— 设置数据缓冲区指针
///     gp.set_data_length(4)   —— 数据长度
///     gp.get_data_ptr()       —— target 通过这个读到数据
///
/// 过程：
///   initiator: WRITE(addr=0x40, data=0xDEADBEEF)
///        | b_transport(gp, delay=0)
///        v
///   target:   收到数据 → 存储到内部 memory → delay = 10ns + 5ns = 15ns
///        | 返回，delay=15ns
///        v
///   initiator: wait(15ns)
///        |
///   initiator: READ(addr=0x40)
///        | b_transport(gp, delay=0)
///        v
///   target:   从内部 memory 取出数据 → 写回 gp → delay = 10ns + 10ns = 20ns
///        | 返回
///        v
///   initiator: 验证读到的数据 == 写入的数据 ✓
//
//==============================================================================

#include <iostream>
#include <cstring>
#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"

//==============================================================================
// ② Target：接收请求 + 存储数据 + 累加延迟
//==============================================================================
class MyTarget : public sc_core::sc_module
{
public:
  tlm_utils::simple_target_socket<MyTarget> socket;

  MyTarget(sc_core::sc_module_name name,
           sc_core::sc_time accept_delay        = sc_core::sc_time(10, sc_core::SC_NS),
           sc_core::sc_time write_response_delay = sc_core::sc_time(5,  sc_core::SC_NS),
           sc_core::sc_time read_response_delay  = sc_core::sc_time(10, sc_core::SC_NS))
    : socket("socket")
    , m_accept_delay(accept_delay)
    , m_write_response_delay(write_response_delay)
    , m_read_response_delay(read_response_delay)
  {
    std::memset(memory, 0, sizeof(memory));
    socket.register_b_transport(this, &MyTarget::b_transport);
  }

private:
  unsigned char memory[256];             // 模拟一块"内存"
  sc_core::sc_time m_accept_delay;       // 每次请求的固定开销
  sc_core::sc_time m_write_response_delay;
  sc_core::sc_time m_read_response_delay;

  void b_transport(tlm::tlm_generic_payload& gp, sc_core::sc_time& delay)
  {
    tlm::tlm_command  cmd  = gp.get_command();
    sc_dt::uint64     addr = gp.get_address();
    unsigned char*    data = gp.get_data_ptr();
    unsigned int      len  = gp.get_data_length();

    std::cout << std::endl;
    std::cout << "  [Target] b_transport 被调用" << std::endl;
    std::cout << "    命令: " << (cmd == tlm::TLM_WRITE_COMMAND ? "WRITE" : "READ")
              << "  地址: 0x" << std::hex << addr << std::dec
              << "  长度: " << len
              << "  入口 delay: " << delay << std::endl;

    // 固定开销
    delay += m_accept_delay;
    std::cout << "    + accept_delay (" << m_accept_delay
              << ") → delay=" << delay << std::endl;

    if (cmd == tlm::TLM_WRITE_COMMAND)
    {
      // 写：initiator 的数据 → target 内部 memory
      std::memcpy(&memory[addr], data, len);
      delay += m_write_response_delay;
      std::cout << "    WRITE: memory[0x" << std::hex << addr << std::dec
                << "] ← 0x" << std::hex
                << *reinterpret_cast<unsigned int*>(data) << std::dec << std::endl;
      std::cout << "    + write_delay (" << m_write_response_delay
                << ") → delay=" << delay << std::endl;
    }
    else
    {
      // 读：target 内部 memory → initiator 的 data buffer
      std::memcpy(data, &memory[addr], len);
      delay += m_read_response_delay;
      std::cout << "    READ:  memory[0x" << std::hex << addr << std::dec
                << "] → 0x" << std::hex
                << *reinterpret_cast<unsigned int*>(data) << std::dec << std::endl;
      std::cout << "    + read_delay (" << m_read_response_delay
                << ") → delay=" << delay << std::endl;
    }

    gp.set_response_status(tlm::TLM_OK_RESPONSE);
  }
};

//==============================================================================
// ① Initiator：发起一次写 + 一次读，验证数据一致性
//==============================================================================
class MyInitiator : public sc_core::sc_module
{
public:
  tlm_utils::simple_initiator_socket<MyInitiator> socket;

  SC_HAS_PROCESS(MyInitiator);
  MyInitiator(sc_core::sc_module_name name) : socket("socket")
  {
    SC_THREAD(run);
  }

private:
  void run()
  {
    unsigned char data_buf[4];
    tlm::tlm_generic_payload gp;

    // ---- WRITE ----
    std::cout << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << "  第 1 步：发起 WRITE 请求" << std::endl;
    std::cout << "============================================" << std::endl;

    unsigned int write_data = 0xDEADBEEF;
    std::memcpy(data_buf, &write_data, 4);

    gp.set_command(tlm::TLM_WRITE_COMMAND);
    gp.set_address(0x40);
    gp.set_data_ptr(data_buf);
    gp.set_data_length(4);
    gp.set_streaming_width(4);
    gp.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

    sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
    socket->b_transport(gp, delay);

    std::cout << std::endl;
    std::cout << "  [Initiator] WRITE 返回" << std::endl;
    std::cout << "    response_status = "
              << (gp.get_response_status() == tlm::TLM_OK_RESPONSE ? "OK" : "ERROR")
              << ", delay = " << delay << std::endl;
    std::cout << "    现在 wait(" << delay << ")..." << std::endl;

    wait(delay);
    std::cout << "    wait 结束，仿真时间现在 = "
              << sc_core::sc_time_stamp() << std::endl;

    // ---- READ（验证） ----
    std::cout << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << "  第 2 步：发起 READ 请求验证数据" << std::endl;
    std::cout << "============================================" << std::endl;

    unsigned char read_buf[4] = {0, 0, 0, 0};

    gp.set_command(tlm::TLM_READ_COMMAND);
    gp.set_address(0x40);
    gp.set_data_ptr(read_buf);
    gp.set_data_length(4);

    delay = sc_core::SC_ZERO_TIME;
    socket->b_transport(gp, delay);

    unsigned int read_data = *reinterpret_cast<unsigned int*>(read_buf);
    std::cout << std::endl;
    std::cout << "  [Initiator] READ 返回" << std::endl;
    std::cout << "    response_status = "
              << (gp.get_response_status() == tlm::TLM_OK_RESPONSE ? "OK" : "ERROR")
              << ", delay = " << delay << std::endl;
    std::cout << "    读到的数据 = 0x" << std::hex << read_data << std::dec << std::endl;

    std::cout << std::endl;
    if (read_data == write_data)
    {
      std::cout << "  ✅ 数据验证通过！read_data == write_data == 0x"
                << std::hex << write_data << std::dec << std::endl;
    }
    else
    {
      std::cout << "  ❌ 数据验证失败！expected=0x" << std::hex << write_data
                << " got=0x" << read_data << std::dec << std::endl;
    }

    wait(delay);
    std::cout << "    wait 结束，仿真时间现在 = "
              << sc_core::sc_time_stamp() << std::endl;
  }
};

//==============================================================================
// ③ sc_main
//==============================================================================
int sc_main(int /*argc*/, char* /*argv*/[])
{
  std::cout << "==============================================" << std::endl;
  std::cout << "  Step 2: delay 时序 + GP 数据传递" << std::endl;
  std::cout << "  目标：理解 target 如何标注延迟、数据如何在 GP 中流动" << std::endl;
  std::cout << "==============================================" << std::endl;

  MyInitiator init("initiator");
  MyTarget    targ("target",
                   sc_core::sc_time(10, sc_core::SC_NS),   // accept_delay
                   sc_core::sc_time(5,  sc_core::SC_NS),   // write_response_delay
                   sc_core::sc_time(10, sc_core::SC_NS));  // read_response_delay

  init.socket(targ.socket);
  sc_core::sc_start();

  std::cout << std::endl;
  std::cout << "==============================================" << std::endl;
  std::cout << "  最终仿真时间: " << sc_core::sc_time_stamp() << std::endl;
  std::cout << "  仿真结束！" << std::endl;
  std::cout << "==============================================" << std::endl;

  return 0;
}
