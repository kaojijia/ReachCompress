#!/bin/bash

# 配置
PID=3479403  # 替换为你要监控的进程PID
LOG_FILE="/home/reco/Projects/ReachCompress/result/memory_log.txt"
INTERVAL=60  # 时间间隔（秒）

# 检查PID是否存在
if ! kill -0 $PID 2>/dev/null; then
    echo "$(date '+%Y-%m-%d %H:%M:%S') - PID=${PID} 不存在。" >> "$LOG_FILE"
    exit 1
fi

while true
do
    if kill -0 $PID 2>/dev/null; then
        # 使用 ps 获取内存使用情况（RSS，单位为KB）
        mem_kb=$(ps -p $PID -o rss=)
        mem_mb=$(awk "BEGIN {printf \"%.2f\", $mem_kb/1024}")
        echo "$(date '+%Y-%m-%d %H:%M:%S') - PID=${PID} - Memory Usage: ${mem_mb} MB" >> "$LOG_FILE"
    else
        echo "$(date '+%Y-%m-%d %H:%M:%S') - PID=${PID} 不存在。" >> "$LOG_FILE"
        exit 1
    fi
    sleep $INTERVAL
done
