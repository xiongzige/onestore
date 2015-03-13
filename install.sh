#!/bash/bin
sudo insmod kernel/btier/btier.ko 
sudo ./cli/btier_setup -f /tmp/ssd50M.img:/tmp/hdd100M.img -c

sudo chown xiongzi /sys/block/sdtiera/tier/*
sudo echo 1 > /sys/block/sdtiera/tier/migrate_verbose 
sudo echo "1 paged" > /sys/block/sdtiera/tier/show_blockinfo
