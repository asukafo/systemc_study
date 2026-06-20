//==============================================================================
/// @file step3.cpp
///
/// @brief TLM-2.0 LT 第 3 步：加入地址路由 Bus
///
/// 在 step2 的基础上加入一个简单的地址解码 Bus：
///   - 1 个 initiator → Bus → 2 个 target
///   - Bus 根据地址范围决定把请求发给哪个 target
///   - 地址 0x000 ~ 0x0FF → Target 0
///   - 地址 0x100 ~ 0x1FF → Target 1
//
//==============================================================================
///
/// ⚠️ 重要教训（调试时发现的 Bug）：
///
///   Bus 会修改 GP 的地址字段（gp.set_address(addr & ADDR_MASK)）。
///   这意味着 b_transport 返回后，GP 里的地址已经不是原始地址了！
///   如果你要复用同一个 GP 对象发下一个请求，必须重新 set_address()，
///   否则下一请求的路由会出错。
///
///   这也是为什么官方代码里 traffic_generator 每次发请求前都完整设置 GP 的所有字段。
///
/// 架构：
///
///                         sc_main
///                           |
///               ┌───────────┼───────────┐
///               v           v           v
///          MyInitiator    MyBus     MyTarget × 2
///               |           |
///          socket ──bind──> target_socket[0]
///                           |
///                           | (地址解码：addr < 0x100 ? target0 : target1)
///                           | (地址掩码：addr & 0xFF → 本地地址)
///                           |
///                   ┌───────┴───────┐
///                   v               v
///          initiator_socket[0]  initiator_socket[1]
///               |                    |
///               | bind               | bind
///               v                    v
///          MyTarget 0          MyTarget 1
///          (地址范围 0x000)    (地址范围 0x100)
///
/// 关键发现：
///   1. Bus 是"两面人"：对上（initiator）它是 target，对下（target）它是 initiator
///   2. 地址解码是在 Bus 内部完成的，initiator 和 target 互不知道对方
///   3. Bus 修改 GP 的地址（mask out 高 bit），让 target 看到本地地址
//
//==============================================================================

#include <iostream>
#include <cstring>
#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"

//==============================================================================
// ③ Target：与 step2 完全相同，不知道 Bus 的存在
//==============================================================================
class MyTarget : public sc_core::sc_module
{
public:
  tlm_utils::simple_target_socket<MyTarget> socket;

  MyTarget(sc_core::sc_module_name name,
           unsigned int id,
           sc_core::sc_time accept_delay = sc_core::sc_time(10, sc_core::SC_NS),
           sc_core::sc_time write_delay  = sc_core::sc_time(5,  sc_core::SC_NS),
           sc_core::sc_time read_delay   = sc_core::sc_time(10, sc_core::SC_NS))
    : socket("socket")
    , m_id(id)
    , m_accept_delay(accept_delay)
    , m_write_delay(write_delay)
    , m_read_delay(read_delay)
  {
    std::memset(memory, 0, sizeof(memory));
    socket.register_b_transport(this, &MyTarget::b_transport);
  }

private:
  unsigned char memory[256];
  unsigned int  m_id;
  sc_core::sc_time m_accept_delay;
  sc_core::sc_time m_write_delay;
  sc_core::sc_time m_read_delay;

  void b_transport(tlm::tlm_generic_payload& gp, sc_core::sc_time& delay)
  {
    tlm::tlm_command  cmd  = gp.get_command();
    sc_dt::uint64     addr = gp.get_address();
    unsigned char*    data = gp.get_data_ptr();
    unsigned int      len  = gp.get_data_length();

    delay += m_accept_delay;

    if (cmd == tlm::TLM_WRITE_COMMAND)
    {
      std::memcpy(&memory[addr], data, len);
      delay += m_write_delay;
      std::cout << "    [Target " << m_id << "] WRITE "
                << "addr=0x" << std::hex << addr << std::dec
                << " data=0x" << std::hex
                << *reinterpret_cast<unsigned int*>(data) << std::dec
                << " delay=" << delay << std::endl;
    }
    else
    {
      std::memcpy(data, &memory[addr], len);
      delay += m_read_delay;
      std::cout << "    [Target " << m_id << "] READ  "
                << "addr=0x" << std::hex << addr << std::dec
                << " data=0x" << std::hex
                << *reinterpret_cast<unsigned int*>(data) << std::dec
                << " delay=" << delay << std::endl;
    }

    gp.set_response_status(tlm::TLM_OK_RESPONSE);
  }
};

//==============================================================================
// ② Bus：地址解码 + 路由
//==============================================================================
class MyBus : public sc_core::sc_module
{
public:
  // ★ Bus 的两面：
  //   对上（initiator），暴露 target_socket —— 接收请求
  //   对下（target），暴露 initiator_socket[2] —— 转发请求

  tlm_utils::simple_target_socket<MyBus>    target_socket;
  tlm_utils::simple_initiator_socket<MyBus> initiator_socket[2];

  MyBus(sc_core::sc_module_name name)
    : target_socket("target_socket")
  {
    target_socket.register_b_transport(this, &MyBus::busBTransport);
  }

private:
  static const sc_dt::uint64 ADDR_MASK  = 0xFF;   // 低 8 位 = target 本地地址
  static const sc_dt::uint64 ADDR_SPLIT = 0x100;  // 第 9 位 = 选 target

  // ---- Bus 的核心：地址解码 + 转发 ----
  void busBTransport(tlm::tlm_generic_payload& gp, sc_core::sc_time& delay)
  {
    sc_dt::uint64 addr       = gp.get_address();
    unsigned int  port       = decode(addr);

    std::cout << "  [Bus] 收到请求 addr=0x" << std::hex << addr << std::dec
              << " → 路由到 port " << port << std::endl;

    // ★ 修改 GP 地址：去掉高位，target 只看到本地地址
    gp.set_address(addr & ADDR_MASK);

    // ★ 转发到对应 target
    initiator_socket[port]->b_transport(gp, delay);

    std::cout << "  [Bus] port " << port
              << " 返回, delay=" << delay << std::endl;
  }

  unsigned int decode(sc_dt::uint64 addr)
  {
    if (addr < ADDR_SPLIT)
      return 0;    // 低位地址 → Target 0
    else
      return 1;    // 高位地址 → Target 1
  }
};

//==============================================================================
// ① Initiator：对两个地址区域各做一次写+读
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
    unsigned char read_buf[4];
    tlm::tlm_generic_payload gp[2];    // 两个 GP，分别用于两个 target
    sc_core::sc_time delay;

    // ---- 访问 Target 0（地址 < 0x100） ----
    std::cout << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << "  访问 Target 0 区域（地址 < 0x100）" << std::endl;
    std::cout << "============================================" << std::endl;

    // WRITE
    unsigned int w0 = 0xAAAA5555;
    std::memcpy(data_buf, &w0, 4);
    gp[0].set_command(tlm::TLM_WRITE_COMMAND);
    gp[0].set_address(0x40);
    gp[0].set_data_ptr(data_buf);
    gp[0].set_data_length(4);
    gp[0].set_streaming_width(4);

    delay = sc_core::SC_ZERO_TIME;
    std::cout << "  [Initiator] → WRITE addr=0x40 data=0x"
              << std::hex << w0 << std::dec << std::endl;
    socket->b_transport(gp[0], delay);
    std::cout << "  [Initiator] ← 返回, delay=" << delay << std::endl;
    wait(delay);

    // READ
    std::memset(read_buf, 0, 4);
    gp[0].set_command(tlm::TLM_READ_COMMAND);
    gp[0].set_address(0x40);       // ★ 重新设置！Bus 在 WRITE 时改了地址
    gp[0].set_data_ptr(read_buf);

    delay = sc_core::SC_ZERO_TIME;
    std::cout << "  [Initiator] → READ  addr=0x40" << std::endl;
    socket->b_transport(gp[0], delay);
    unsigned int r0 = *reinterpret_cast<unsigned int*>(read_buf);
    std::cout << "  [Initiator] ← 返回, data=0x" << std::hex << r0 << std::dec
              << ", delay=" << delay << std::endl;
    std::cout << "  " << (r0 == w0 ? "✅ 数据正确" : "❌ 数据错误") << std::endl;
    wait(delay);

    // ---- 访问 Target 1（地址 >= 0x100） ----
    std::cout << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << "  访问 Target 1 区域（地址 >= 0x100）" << std::endl;
    std::cout << "============================================" << std::endl;

    // WRITE
    unsigned int w1 = 0xBEEF0001;
    std::memcpy(data_buf, &w1, 4);
    gp[1].set_command(tlm::TLM_WRITE_COMMAND);
    gp[1].set_address(0x140);
    gp[1].set_data_ptr(data_buf);
    gp[1].set_data_length(4);
    gp[1].set_streaming_width(4);

    delay = sc_core::SC_ZERO_TIME;
    std::cout << "  [Initiator] → WRITE addr=0x140 data=0x"
              << std::hex << w1 << std::dec << std::endl;
    socket->b_transport(gp[1], delay);
    std::cout << "  [Initiator] ← 返回, delay=" << delay << std::endl;
    wait(delay);

    // READ
    std::memset(read_buf, 0, 4);
    gp[1].set_command(tlm::TLM_READ_COMMAND);
    gp[1].set_address(0x140);      // ★ 重新设置！Bus 在 WRITE 时改了地址
    gp[1].set_data_ptr(read_buf);

    delay = sc_core::SC_ZERO_TIME;
    std::cout << "  [Initiator] → READ  addr=0x140" << std::endl;
    socket->b_transport(gp[1], delay);
    unsigned int r1 = *reinterpret_cast<unsigned int*>(read_buf);
    std::cout << "  [Initiator] ← 返回, data=0x" << std::hex << r1 << std::dec
              << ", delay=" << delay << std::endl;
    std::cout << "  " << (r1 == w1 ? "✅ 数据正确" : "❌ 数据错误") << std::endl;
    wait(delay);
  }
};

//==============================================================================
// ④ sc_main：initiator → bus → target0, target1
//==============================================================================
int sc_main(int /*argc*/, char* /*argv*/[])
{
  std::cout << "==============================================" << std::endl;
  std::cout << "  Step 3: 加入地址路由 Bus" << std::endl;
  std::cout << "  目标：理解 Bus 如何做地址解码和请求转发" << std::endl;
  std::cout << "==============================================" << std::endl;

  MyInitiator init("initiator");
  MyBus       bus ("bus");
  MyTarget    targ0("target_0", 0);   // ID=0，接收 addr < 0x100
  MyTarget    targ1("target_1", 1);   // ID=1，接收 addr >= 0x100

  init.socket(bus.target_socket);
  bus.initiator_socket[0](targ0.socket);
  bus.initiator_socket[1](targ1.socket);

  sc_core::sc_start();

  std::cout << std::endl;
  std::cout << "==============================================" << std::endl;
  std::cout << "  最终仿真时间: " << sc_core::sc_time_stamp() << std::endl;
  std::cout << "  仿真结束！" << std::endl;
  std::cout << "==============================================" << std::endl;

  return 0;
}
