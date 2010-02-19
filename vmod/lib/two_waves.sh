#!      /bin/sh

python 1xa2.py --device 12a2 --lun 1 --channel 0  --period 1.72 --sampling 100 &
python 1xa2.py --device 12a2 --lun 1 --channel 1  --period 1.53 --sampling 120 &

