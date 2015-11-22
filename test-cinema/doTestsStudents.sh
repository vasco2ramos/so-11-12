#!/bin/bash


success=0
fail=0

## Concurrency levels:
### C0: - 1 client - 1 server
### C1: - 10 client - 10 client servers

C[0]="1 1"
Ct[0]="No concurrency: 1 client - 1 server"
C[1]="10 10"
Ct[1]="10 clients - 10 servers"

## Seat choices && Map size:
### S1: 1 seat per reservation - no conflict - 10x10
### S2: 1 seat per reservation - random conflict - 10x10 

S[1]="1 0 10 10"
St[1]="1 seat per reservation - no conflict - 10x10"
S[2]="1 1 10 10"
St[2]="1 seat per reservation - random conflict - 10x10"

## Operations
### ORc:  reservation & snapshot & cancel
### OPCc: prereservation & confirm & cancel

O[1]="0"
Ot[1]="reservation - snapshot - cancel"
O[2]="4"
Ot[2]="prereservation - confirm - cancel"



###1st batch - 1 request per client - CONFLICT FREE: C0xS1x(ORc+OPCc)=2 tests 
# Invariances checked:
# - No aborts ever
# - 1 only request per client => the output must comply with that generated by the reference implementation with the exeption of the reservation ids that may be different

c=0
s=1
snapshot=1
for op in 1 2
do
	echo ---------------------------------------------------
	echo -e "Testing:\n- ${Ct[$c]}\n- ${St[$s]}\n- ${Ot[$op]}\n- 1 req per client\n"

	rm tmp tmp1 &>/dev/null

	./test-res-students ${O[$op]} ${C[$c]} 1000 ${S[$s]} 1 200 ${snapshot} >> tmp

	sed -e 's/{ Res:[0-9]* #seats:1/{ Res:X #seats:1/;s/<R: *[0-9]*|/<R:  X|/' tmp > tmp1

	sed -e 's/{ Preres:[0-9]* #seats:1/{ Preres:X #seats:1/;s/<P: *[0-9]*|/<P:  X|/' tmp1 > out_C${c}_S${s}_O${op} 

	

	diff out_C${c}_S${s}_O${op} expected/C${c}_S${s}_O${op}


	if [ $? = 0 ]
	then 
		echo success 
		let success++
	else 
		echo fail
		let fail++
	 fi
	
	echo ---------------------------------------------------
	echo  
done




###2nd batch - 100 reqs per client 
### CONFLICT PRONE: C1xS2x(ORc+OPCc)=2 tests 
# Invariances checked:
## - no deadlocks
## - at the end of the test there must be no reservations

snapshot=0
numReqs=100
c=1
s=2
for op in 1 2
do
	echo ---------------------------------------------------
	echo -e "Testing:\n- ${Ct[$c]}\n- ${St[$s]}\n- ${Ot[$op]}\n- 100 reqs per client\n"


	rm tmp tmp1 &>/dev/null
	


	./test-res-students ${O[$op]} ${C[$c]} 1000 ${S[$s]} ${numReqs} 200 ${snapshot} >> tmp

	echo "Failed ops (should be less than 100):"
	grep "#Fail" tmp 
	echo ------------

	tail -n 32 tmp  | head -n 30 > tmp1 #tmp gets the map of the cinema which must be empty
	
	diff tmp1 expected/C${c}_S${s}_O${op}



	if [ $? = 0 ]
	then 
		echo success 
		let success++
	else 
		echo fail
		let fail++
	 fi


	echo ---------------------------------------------------
	echo 
	

done

	echo -----------------  TOTAL STATS  ----------------------


echo S:${success} F:${fail}