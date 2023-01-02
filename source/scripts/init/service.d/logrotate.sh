#!/bin/sh

source /etc/device.properties

if [ "$BOX_TYPE" == "HUB4" ] || [ "$BOX_TYPE" == "SR213" ]; then
source /etc/utopia/service.d/log_capture_path.sh
VARLOG_DIR_THRESHOLD=3000
VAR_TMP_FILE_THRESHOLD=1000
tmp_file_size=0
else
VARLOG_DIR_THRESHOLD=5000
fi

#Save the filename which is emptied last with time
save_file=/tmp/.logrotation.txt

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
       if [ -n "$i" ]; then
          if [ "$BOX_TYPE" == "HUB4" ]; then
             #tail last 100lines of log from biggest size file
             if [ -n "$bfile" ] && [ $bsize -gt 0 ] && [ "$i" == "$bfile" ]; then
                echo_t "************* RDKB_VAR_LOG_FILE_NULLIFY ***********************"
                echo "File:$bfile Size:$bsize"
                echo_t "*********************** File Content **************************"
                tail -n 100 "/var/log/$bfile"
                echo_t "***************************************************************"
             fi
          fi

          cat /dev/null > /var/log/"$i"
          echo "[logrotate.sh] Emptied /var/log/$i at $(date)" > $save_file
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
                    echo "[logrotate.sh] Emptied /var/log/$file at $(date)" > $save_file
            else
                    cat /dev/null > /var/log/"$file"
                    echo "[logrotate.sh] Emptied /var/log/$file at $(date)" > $save_file
            fi
        fi
     done
fi

# If any file reaches 1MB inside /var/tmp, empty the file.
if [ "$BOX_TYPE" == "HUB4" ] || [ "$BOX_TYPE" == "SR213" ]; then
    var_tmp_files=`ls /var/tmp/`
    for tmp_file in $var_tmp_files; do
        tmp_file_size=`du /var/tmp/$tmp_file | awk -v sum=0 '{print sum+=$1}' | tail -1`
        if [ $tmp_file_size -gt $VAR_TMP_FILE_THRESHOLD ]; then
            cat /dev/null > /var/tmp/$tmp_file
            echo "[logrotate.sh] Emptied /var/tmp/$tmp_file at $(date)" > $save_file
        fi
    done
fi

exit 0
