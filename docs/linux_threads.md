# 线程状态
- 运行：Running
- 就绪：Ready或Runnable
- 阻塞：Blocked或Waiting
或
- 新建：New
- 就绪：Ready/Runnable
- 运行：Running
- 阻塞：Blocked / Waiting
- 终止：Terminated

```text
                 时间片用完或被抢占
             ┌─────────────────────┐
             ↓                     │
新建 ──────→ 就绪 ──────调度──────→ 运行 ──────执行结束─────→ 终止
             ↑                     │
             │                     │ 等待锁、I/O、条件等
             │                     ↓
             └──────事件完成────── 阻塞
                                       
```