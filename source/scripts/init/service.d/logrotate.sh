 #!/bin/sh

dir=`du /var/log/ | awk -v sum=0 '{print sum+=$1}' | tail -1`

ksize=0
i=0
kfile[2]=""
    file_list=`ls /var/log/`
    for file in $file_list
      do
                if [ "$file" == "kernel" ] || [ "$file" == "user" ] || [ "$file" == "kernel.log" ] || [ "$file" == "user.log" ];  then
                        kfile[$i]="$file"
                        size=`du /var/log/$file | awk -v sum=0 '{print sum+=$1}' | tail -1`
                        ksize=`expr $ksize + $size`
                        i=`expr $i + 1`
            fi
        done

dir=`expr $dir - $ksize`

if [ $ksize -gt 5000 ]; then
    cat /dev/null > /var/log/${kfile[0]}
    cat /dev/null > /var/log/${kfile[1]}
fi

if [ $dir -gt 5000 ]; then
    file_list=`ls /var/log/`
    for file in $file_list
      do
        if [ "$file" != "kernel" ] && [ "$file" != "user" ] && [ "$file" != "kernel.log" ] && [ "$file" != "user.log" ];  then
            if [ "$file" == "dibbler" ];  then
                    cat /dev/null > /var/log/dibbler/dibbler-client.log
                    cat /dev/null > /var/log/dibbler/dibbler-server.log
            else
                    cat /dev/null > /var/log/$file
            fi
        fi
     done
fi

exit 0
