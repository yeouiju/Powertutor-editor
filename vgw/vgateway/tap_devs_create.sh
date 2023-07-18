
sudo tunctl -t tap-left -u myoungwoo
sudo tunctl -t tap-right -u myoungwoo

sudo ifconfig tap-right hw ether 00:11:22:33:44:66 up
sudo ifconfig tap-left hw ether 00:11:22:33:44:55 up
