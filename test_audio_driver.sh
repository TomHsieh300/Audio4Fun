#!/bin/bash
sox -n -r 48000 -c 2 -b 16 /tmp/source.wav trim 0 5 2>/dev/null || \
    dd if=/dev/zero of=/tmp/source.raw bs=1024 count=100

echo "Starting Robust Stress Test..."

for i in {1..100}
do
    echo "=== Run #$i ==="

    arecord -D hw:2,0 -f S16_LE -r 48000 -c 2 /tmp/rec_test.wav &
    REC_PID=$!
    
    aplay -D hw:2,0 -f S16_LE -r 48000 -c 2 /tmp/source.raw &
    PLAY_PID=$!

    sleep 0.1

    kill -SIGINT $REC_PID
    
    kill -SIGINT $PLAY_PID

    wait $REC_PID 2>/dev/null
    wait $PLAY_PID 2>/dev/null

    sleep 0.1
done

echo "Stress Test Completed. System is alive!"
