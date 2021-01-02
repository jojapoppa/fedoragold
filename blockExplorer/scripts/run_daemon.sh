cd /root
rm fedoragold_daemon.logold
mv fedoragold_daemon.log fedoragold_daemon.logold
touch fedoragold_daemon.log
./fedoragold_daemon --log-level 2 --rpc-bind-ip 0.0.0.0 --data-dir /root/.fedoragold
