//==============================================================================
/// @file step1.cpp
///
/// @brief TLM-2.0 LT 第 1 步：最简 b_transport
///
/// 全文件只有一个 initiator、一个 target、一次 b_transport 调用。
/// 目标：理解 TLM-2.0 的核心调用链 —— socket → b_transport → target 方法。
//
//==============================================================================
///
/// 架构（代码从上到下按调用顺序排列）：
///
///   sc_main
///     |
///     ├─ 实例化 MyInitiator
///     ├─ 实例化 MyTarget
///     ├─ init.socket(targ.socket)  ← 绑定！socket 对 socket 直接连接
///     └─ sc_start()               ← 启动仿真
///           |
///           └─ MyInitiator::run()   ← SC_THREAD 自动执行
///                 |
///                 ├─ 创建一个 tlm_generic_payload（GP）对象
///                 ├─ 设置命令=WRITE，地址=0x100
///                 ├─ socket->b_transport(gp, delay)   ← ★ 核心调用
///                 │     |
///                 │     └─ → MyTarget::b_transport(gp, delay)
///                 │             |
///                 │             ├─ 打印收到的地址和命令
///                 │             └─ 设置 response_status = OK
///                 │
///                 └─ 打印完成信息
///
/// 关键发现：
///   1. b_transport 不是"发消息"，而是直接调用 target 的方法（函数调用语义）
///   2. initiator 调用 b_transport 后会阻塞，直到 target 返回
///   3. tlm_generic_payload 是贯穿始终的事务对象
//
//==============================================================================

#include <iostream>
#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"

//==============================================================================
// ② Target（被动方）：等待 initiator 调用，收到请求后处理并返回
//==============================================================================
class MyTarget : public sc_core::sc_module
{
public:
  // Target 的 TLM socket —— 模板参数是"父模块类型"
  tlm_utils::simple_target_socket<MyTarget> socket;

  MyTarget(sc_core::sc_module_name name)
    : socket("socket")
  {
    // ★ 注册回调：有人调 b_transport 时请调用我的 b_transport 方法
    socket.register_b_transport(this, &MyTarget::b_transport);
  }

private:
  // ---- b_transport：TLM-2.0 核心接口 ----
  // gp    —— 事务对象引用（命令、地址、数据）
  // delay —— 延迟时间引用，target 可以往里面加时间
  void b_transport(tlm::tlm_generic_payload& gp, sc_core::sc_time& delay)
  {
    std::cout << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << "  Target::b_transport 被调用了！"          << std::endl;
    std::cout << "  Target 收到 地址 = 0x"
              << std::hex << gp.get_address() << std::dec   << std::endl;
    std::cout << "  Target 收到 命令 = "
              << (gp.get_command() == tlm::TLM_WRITE_COMMAND
                     ? "WRITE (写)" : "READ (读)")          << std::endl;
    std::cout << "  当前 delay = " << delay                  << std::endl;
    std::cout << "============================================" << std::endl;

    gp.set_response_status(tlm::TLM_OK_RESPONSE);
  }
};

//==============================================================================
// ① Initiator（主动方）：创建事务对象，发起 b_transport 调用
//==============================================================================
class MyInitiator : public sc_core::sc_module
{
public:
  tlm_utils::simple_initiator_socket<MyInitiator> socket;

  SC_HAS_PROCESS(MyInitiator);
  MyInitiator(sc_core::sc_module_name name)
    : socket("socket")
  {
    SC_THREAD(run);
  }

private:
  void run()
  {
    // ① 在栈上创建 generic payload（GP）—— TLM-2.0 的事务载体
    tlm::tlm_generic_payload gp;

    // ② 设置事务参数
    gp.set_command(tlm::TLM_WRITE_COMMAND);
    gp.set_address(0x100);

    // ③ delay 从 0 开始
    sc_core::sc_time delay = sc_core::SC_ZERO_TIME;

    // ★★★ 核心调用：b_transport ★★★
    // socket 通过"绑定关系"找到连着的 target socket，调用其注册的回调。
    // 在 LT 模式下，一次 b_transport 完成整个事务。
    std::cout << std::endl;
    std::cout << "Initiator: 准备发起 b_transport..." << std::endl;
    std::cout << "  GP 命令 = WRITE" << std::endl;
    std::cout << "  GP 地址 = 0x" << std::hex << 0x100 << std::dec << std::endl;

    socket->b_transport(gp, delay);

    // ④ 检查返回结果
    std::cout << std::endl;
    std::cout << "Initiator: b_transport 返回了！" << std::endl;
    std::cout << "  Response Status = "
              << (gp.get_response_status() == tlm::TLM_OK_RESPONSE
                     ? "OK" : "ERROR") << std::endl;
    std::cout << "  delay = " << delay << std::endl;

    wait(delay);
  }
};

//==============================================================================
// ③ sc_main：SystemC 入口点
//==============================================================================
int sc_main(int /*argc*/, char* /*argv*/[])
{
  std::cout << "==============================================" << std::endl;
  std::cout << "  Step 1: 最简 b_transport" << std::endl;
  std::cout << "  目标：理解 socket → b_transport 调用链" << std::endl;
  std::cout << "==============================================" << std::endl;

  MyInitiator init("initiator");
  MyTarget    targ("target");

  // ★ 绑定：initiator socket → target socket
  init.socket(targ.socket);

  sc_core::sc_start();

  std::cout << std::endl;
  std::cout << "==============================================" << std::endl;
  std::cout << "  仿真结束！" << std::endl;
  std::cout << "==============================================" << std::endl;

  return 0;
}
