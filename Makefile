# Makefile

# compile all codes
all:
	gcc -o serverA serverA.c
	gcc -o serverB serverB.c
	gcc -o aws aws.c
	gcc -o client client.c

# run serverA
.PHONY: serverA
serverA:
	./serverA

#run serverB
.PHONY: serverB
serverB:
	./serverB

#run aws and avoid: 'aws' is up to date
.PHONY: aws
aws:
	./aws
