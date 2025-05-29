            #!/bin/sh
            LOG_FOLDER="/tmp/valgrind"
            SUMMARY_FILE=$GITHUB_STEP_SUMMARY
            LEAKS_FOUND=false
            for LOG_FILE in $LOG_FOLDER/*.log; do
             echo "iterating through $LOG_FILE"
             if ( grep -q "ERROR SUMMARY" $LOG_FILE  && ! grep -q "ERROR SUMMARY: 0 errors" $LOG_FILE ); then
             LEAKS_FOUND=true
             echo "leak found set to  $LEAK_FOUND"
             echo "Running logfile : $LOG_FILE" >>$SUMMARY_FILE
             BINARY_PATH=$(basename $LOG_FILE .log)
             echo "Running bin : $BINARY_PATH" >>$SUMMARY_FILE
             #EXTRACT THE RELEVANT INFO
             LEAK_SUMMARY=$(grep -A 5 "LEAK SUMMARY:" $LOG_FILE)
             ERROR_SUMMARY=$(grep -A 1 "ERROR SUMMARY:" $LOG_FILE)
             HEAP_SUMMARY=$(grep -A 5 "HEAP SUMMARY:" $LOG_FILE)
              if [ -n "$LEAK_SUMMARY" ];then
              echo " LEAK Summary" >>$SUMMARY_FILE
              echo "$LEAK_SUMMARY"  >>$SUMMARY_FILE
              fi
              if [ -n "$ERROR_SUMMARY" ];then
              echo "ERROR Summary" >>$SUMMARY_FILE
              echo "$ERROR_SUMMARY"  >>$SUMMARY_FILE
              fi
              if [ -n "$HEAP_SUMMARY" ]; then
              echo "HEAP SUMMARY" >>$SUMMARY_FILE
              echo "$HEAP_SUMMARY" >>$SUMMARY_FILE
              fi
              #cat $LOG_FILE >>$SUMMARY_FILE
             echo "*************************" >>$SUMMARY_FILE
            fi
            done
            #echo "leak found set to  $LEAKS_FOUND outside for"
            if [ "$LEAKS_FOUND" = true ]; then
              echo "inside leak found ----"
              rm -rf /tmp/valgrind/*
              ls -lt /tmp/valgrind
              exit 1
            else
              echo "no leaks found"
            fi
