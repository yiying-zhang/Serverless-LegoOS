# LegoOS State Management System

This document describes usage and setup for state management system.

## Debug Options
Use `CONFIG_DEBUG_STATE=y` in configs to enable debugging information on processor and/or memory nodes.

## Testing Environment
State management has been testing on two set of infrastructures: 1P-1M and 1P-1M-GMM.

For best performance, use multiple vCPU for VM and multiple workers on mComp
For example in memory node config, use
```
CONFIG_THPOOL_NR_WORKERS=8
```

### 1P-1M setup
1P-1M is the simplest setting to test state management system. Use sample config file provided to boot up 1P-1M.

Processor side
```
cp Documentation/configs/1P-1M-Processor .config
```

Memory side
```
cp Documentation/configs/1P-1M-Memory .config
```

Under 1P-1M, Global Memory Monitor (GMM) is not required. In config files, turn off option on processor side:
```
# CONFIG_USE_GMM is not set
```
and on memory side:
```
# CONFIG_GMM is not set
```
### 1P-3M-GMM setup
1P-3M-GMM setting requires 5 nodes. provides sample configs in Documentation/state/configs

- Processor and Memory nodes are running in VMs, while global resource monitor runs on physical machine.
- By default, node 0 is pComp, node 1~3 are mComp, node 4 is Global Resource Monitor.
- Global Resource Monitor requires linux kernel 3.11.1.

Processor side
```
cp Documentation/state/configs/1P-3M-GMM-Processor .config
```

Memory side
```
# node 1
cp Documentation/state/configs/1P-3M-GMM-Memory-1 .config
# node 2
cp Documentation/state/configs/1P-3M-GMM-Memory-2 .config
# node 3
cp Documentation/state/configs/1P-3M-GMM-Memory-3 .config
```

Global Resource Monitor
```
cp Documentation/state/configs/fit_config.h linux-modules/fit/fit_config.h
cp Documentation/state/configs/monitor_config.h linux-modules/include/monitor_config.h
```

- Update Local ID from `ibv_devinfo | grep port_lid` on each node
    - Update `/net/lego/fit_machine.c` with LID on processor and memory nodes.
    - Update `/linux-modules/fit/fit_machine.c` with LID on Global Resource Monitor.

In each of the Processor and Memory node, do
```
make
make install
grub2-mkconfig > /boot/grub2/grub.cfg
```

In Global Resource Monitor, do
```
cd linux-module
make
```

Reboot pComp and mComp nodes into LegoOS, when all of the processors and memory nodes reach `fit: Please wait for enough MAD...`, do
```
sudo make fit_install; sudo make monitor_install
```
The first command will return once the node joined the cluster, and the second command starts the monitors.

## Infiniband Setup on Ubuntu
Referencing instruction from Mellanox
`https://community.mellanox.com/s/article/howto-setup-rdma-connection-using-inbox-driver--rhel--ubuntu-x`

First, install packages
```
apt-get install libmlx4-1 infiniband-diags ibutils ibverbs-utils rdmacm-utils perftest
```
If drivers and kernel modules are not enabled automatically, do the following
- In case `No IB devices found`, do `modprobe mlx4_ib` to enable infiniband driver.
- Do `modprobe ib_umad` to enable `ibstat`.
- Do `modprobe ib_uverbs` to enable `ibv_devinfo`, which is require to obtain LID.

