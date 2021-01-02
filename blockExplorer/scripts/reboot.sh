#!/bin/bash

if (http --timeout 60 127.0.0.1 | grep "Peers")
then

echo -e "checkhttp.sh: $DATE: re-Starting server after backing up bootstrap"
/bin/bash /root/stop_daemon.sh &
timeout 180 tail -f fedoragold_daemon.log

echo 'update bootstrap..'
cp /root/.fedoragold/* /var/www/html/bootstrap
echo 'reboot now...'
sudo /sbin/reboot

else

killall -9 fedoragold_daemon

echo 'reboot now... will boot with old bootstrap files...'
cp /var/www/html/bootstrap/* /root/.fedoragold
sudo /sbin/reboot

fi
