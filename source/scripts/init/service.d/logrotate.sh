#!/bin/sh

source /etc/device.properties

if [ "$BOX_TYPE" == "HUB4" ]; then
source /etc/utopia/service.d/log_capture_path.sh
VARLOG_DIR_THRESHOLD=3000
else
VARLOG_DIR_THRESHOLD=5000
fi

dir=`du /var/log/ | awk -v sum=0 '{print sum+=$1}' | tail -1`

ksize=0
i=0
bsize=0
bfile=""

#4 files mentioned for kernel, user, kernel.log, user.log
kfile[4]=""

    file_list=`ls /var/log/`
    for file in $file_list
      do
                if [ "$file" == "kernel" ] || [ "$file" == "user" ] || [ "$file" == "kernel.log" ] || [ "$file" == "user.log" ];  then
                        kfile[$i]="$file"
                        size=`du /var/log/"$file" | awk -v sum=0 '{print sum+=$1}' | tail -1`
                       
                        #Find biggest file of those kernel files
                        if [ "$BOX_TYPE" == "HUB4" ]; then
                           if [ $size -gt $bsize ]; then
                              bsize=$size
                              bfile="$file"
                           fi
                        fi

                        ksize=`expr $ksize + "$size"`
                        i=`expr $i + 1`
            fi
        done

dir=`expr "$dir" - "$ksize"`

if [ $ksize -gt $VARLOG_DIR_THRESHOLD ]; then
    #Needs to clear all the kernel files
    for i in "${kfile[@]}"
    do
       if [ "$i" != "" ]; then
          if [ "$BOX_TYPE" == "HUB4" ]; then
             #tail last 100lines of log from biggest size file
             if [ "$bfile" != "" ] && [ $bsize -gt 0 ] && [ "$i" == "$bfile" ]; then
                echo_t "************* RDKB_VAR_LOG_FILE_NULLIFY ***********************"
                echo "File:$bfile Size:$bsize"
                echo_t "*********************** File Content **************************"
                tail -n 100 "/var/log/$bfile"
                echo_t "***************************************************************"
             fi
          fi

          cat /dev/null > /var/log/"$i"
       fi  
    done
fi

if [ $dir -gt $VARLOG_DIR_THRESHOLD ]; then
    file_list=`ls /var/log/`
    for file in $file_list
      do
        if [ "$file" != "kernel" ] && [ "$file" != "user" ] && [ "$file" != "kernel.log" ] && [ "$file" != "user.log" ];  then
            if [ "$file" == "dibbler" ];  then
                    cat /dev/null > /var/log/dibbler/dibbler-client.log
                    cat /dev/null > /var/log/dibbler/dibbler-server.log
            else
                    cat /dev/null > /var/log/"$file"
            fi
        fi
     done
fi

exit 0
