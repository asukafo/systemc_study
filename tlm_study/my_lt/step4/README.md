# Step 4: 回到官方示例 —— 概念映射指南

现在你已经亲手写过 `b_transport`、`delay`、`GP` 数据传递、Bus 路由。
把这些概念映射回 `tlm_study/tlm/` 的官方示例。

---

## 映射表：你的代码 → 官方示例

| 你的代码                       | 官方示例对应文件                                           | 多了什么工程化内容                         |
|-------------------------------|----------------------------------------------------------|------------------------------------------|
| `MyInitiator::run()`          | [lt_initiator.cpp](../../tlm/common/src/lt_initiator.cpp) | 用 `sc_fifo` 收发 GP，两个 socket         |
| `MyTarget::b_transport()`     | [lt_target.cpp](../../tlm/common/src/lt_target.cpp)       | target 内部调用 `memory` 类操作            |
| —                             | [at_target_1_phase.cpp](../../tlm/common/src/at_target_1_phase.cpp) | 同时实现 `nb_transport_fw`（AT 协议）    |
| `MyBus::busBTransport()`      | [SimpleBusLT.h](../../tlm/common/inc/models/SimpleBusLT.h) | 模板化多对多路由 + DMI                    |
| —                             | [traffic_generator.cpp](../../tlm/common/src/traffic_generator.cpp) | GP 对象池 + 测试序列 + 结果校验           |
| —                             | [initiator_top.cpp](../../tlm/lt/src/initiator_top.cpp)   | 把 traffic_gen + lt_initiator 封装在一起   |
| `sc_main` 里的绑定            | [lt_top.cpp](../../tlm/lt/src/lt_top.cpp)                 | 模板参数化的多对多拓扑                     |

---

## 逐文件阅读指南

### 1. 先看 `lt_initiator.cpp`（对标你的 step2 initiator）

```cpp
// 你的代码：
socket->b_transport(gp, delay);
wait(delay);

// 官方代码 (lt_initiator.cpp:68)：
initiator_socket->b_transport(*transaction_ptr, delay);
wait(delay);
```

**唯一区别**：官方的 GP 对象是从 `request_in_port->read()` 拿到的（traffic_generator 通过 sc_fifo 发过来），
处理完后通过 `response_out_port->write()` 还回去。你的代码是直接在 run() 里创建的。

### 2. 再看 `lt_target.cpp`（对标你的 step2 target）

```cpp
// 你的代码：
delay += m_accept_delay + m_write_response_delay;
memcpy(&memory[addr], data, len);

// 官方代码 (lt_target.cpp:65,71)：
m_target_memory.operation(payload, mem_op_time);  // 把 memory 操作封装成独立类
delay_time = delay_time + m_accept_delay + mem_op_time;
```

**唯一区别**：官方把 memory 读写逻辑抽到 `memory.cpp` 里，target 只是"壳"。

### 3. 再看 `SimpleBusLT.h`（对标你的 step3 Bus）

```cpp
// 你的代码：
unsigned int decode(uint64 addr) {
    return (addr < 0x100) ? 0 : 1;
}

// 官方代码 (SimpleBusLT.h:45-48)：
unsigned int getPortId(const sc_dt::uint64& address) {
    return (unsigned int)address >> 28;   // 用高 4 位做路由
}
```

**唯一区别**：官方用模板参数 `<NR_OF_INITIATORS, NR_OF_TARGETS>` 支持任意数量的 initiator/target，
你的 bus 是硬编码的 2 对 2。路由策略不同（高位 vs 第 9 位），但原理一样。

### 4. 最后看 `lt_top.cpp`（对标你的 step3 sc_main 绑定）

```cpp
// 你的代码：
init.socket(bus.target_socket);
bus.initiator_socket[0](targ0.socket);

// 官方代码 (lt_top.cpp:70-74)：
m_initiator_1.top_initiator_socket(m_bus.target_socket[0]);
m_bus.initiator_socket[0](m_at_and_lt_target_1.m_memory_socket);
```

**唯一区别**：官方是 2 initiator × 2 target，你的 step3 是 1 × 2。绑定语法完全相同。

---

## 官方示例多出来的部分（读完上面 3 个文件后再看）

### traffic_generator（可以先跳过）

这是一个**纯测试辅助工具**，和 TLM-2.0 协议无关：
- 用 `sc_fifo` 与 lt_initiator 通信（不是 TLM socket）
- 自动生成 WRITE + READ 序列
- 校验读回数据是否正确
- GP 对象池（memory management）

**你不看它也不影响理解 TLM-2.0。**

### initiator_top（可以先跳过）

这是一个**纯包装层**，只是为了把 `traffic_generator + sc_fifo + lt_initiator` 封装成一个模块。
这样 `lt_top` 只需实例化 2 个 `initiator_top` 而不是 6 个小模块。

**你不看它也不影响理解 TLM-2.0。**

### at_target_1_phase（进阶内容）

这个 target 同时实现了 `b_transport`（LT 协议）和 `nb_transport_fw`（AT 协议）。
AT 协议涉及 phase、timing point、多阶段调用，是 LT 的进阶。
**先彻底搞懂 LT 再碰。**

---

## 推荐阅读顺序

```
第一步：你已经看过的
  ├── lt/src/lt.cpp          (sc_main，已理解)
  ├── lt/src/lt_top.cpp      (拓扑绑定，已理解)
  ├── common/inc/models/SimpleBusLT.h  (Bus 路由，已理解)

第二步：对应你的 step1+2 核心
  ├── common/inc/lt_initiator.h   → 然后 common/src/lt_initiator.cpp
  ├── common/inc/lt_target.h      → 然后 common/src/lt_target.cpp
  └── common/inc/memory.h         → 然后 common/src/memory.cpp

第三步：辅助模块（了解即可）
  ├── common/inc/traffic_generator.h → common/src/traffic_generator.cpp
  ├── lt/inc/initiator_top.h        → lt/src/initiator_top.cpp
  └── common/inc/reporting.h        → common/src/report.cpp

第四步：进阶
  └── common/inc/at_target_1_phase.h → common/src/at_target_1_phase.cpp
```

---

## 实验建议

当你熟悉了所有文件后，试试这些修改来验证理解：

1. **改 delay 参数**：在 `lt_top.cpp` 中修改 target 的 `accept_delay`/`read_response_delay`/`write_response_delay`，跑 `make check` 看 diff
2. **改地址路由策略**：在 `SimpleBusLT.h` 中改用低 8 位路由（`address & 0xFF`），看 traffic_generator 的校验是否还通过
3. **去掉一个 traffic_generator**：在 `lt_top.cpp` 里注释掉 `m_initiator_2`，只留一个 initiator，看仿真是否仍然正常
