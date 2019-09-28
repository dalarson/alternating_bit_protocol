Instructions to run:

	make
	./p2

Instructions to clean directory:

	make clean

NOTE:

I'm not sure if other students ran into this issue, but I found that frequently when using an average time between message transmissions of 1000
(as suggested by project description) I occassionally get a message send down from layer 5 before the ACK from the previous message has been
received. In this case, my program breaks. However, since the description says to choose a value large enough that we don't run into this problem,
I have bumped this value up to 10000 or even 100000 to minimize the probability of this happening (although doing so doesn't eliminate the problem,
only decreases the likelihood of it occurring).


