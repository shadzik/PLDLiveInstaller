#!/bin/bash

for class in 0106 0107 0100 0101; do
	module="$module $(lspci -n |grep $class |sed 's/:/ /g' |while read junk junk class vendor device junk; do grep "0x0000$vendor 0x0000$device" /lib/modules/$(uname -r)/modules.pcimap |awk '{print $1}'; done |uniq)"
done

echo $module
