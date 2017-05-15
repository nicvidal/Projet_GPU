export OMP_NUM_THREADS

ITE=$(seq 3) 
THREADS=$(seq 2 2 24)

PARAM="-n -t 500 -s 4096 SIMD"

EXE="./prog -ft -v 4 $PARAM"
OUTPUT="$(echo -n $EXE | tr -d ' ')"

for nb in $ITE; do for OMP_NUM_THREADS in $THREADS; do  echo -n $OMP_NUM_THREADS " " >> $OUTPUT ; $EXE 2>> $OUTPUT; done; done

EXE="./prog  -v 4 $PARAM"
OUTPUT=$(echo -n $EXE | tr -d ' ')


for nb in $ITE; do for OMP_NUM_THREADS in $THREADS; do  echo -n $OMP_NUM_THREADS " " >> $OUTPUT ; $EXE 2>> $OUTPUT; done; done

EXE="./prog  -v 3 $PARAM"
OUTPUT=$(echo -n $EXE | tr -d ' ')

for nb in $ITE; do for OMP_NUM_THREADS in $THREADS; do  echo -n $OMP_NUM_THREADS " " >> $OUTPUT ; $EXE 2>> $OUTPUT; done; done

EXE="./prog -ft -v 3 $PARAM"
OUTPUT=$(echo -n $EXE | tr -d ' ')

for nb in $ITE; do for OMP_NUM_THREADS in $THREADS; do  echo -n $OMP_NUM_THREADS " " >> $OUTPUT ; $EXE 2>> $OUTPUT; done; done
