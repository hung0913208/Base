#!/bin/bash
# - File: enqueue.sh
# - Description: This bash script is performed directly by CI and it will start
# an enqueue locally. Recently, we should only run this script on a single node
# but i'm consider design this script to be runing on multi-nodes.

# @NOTE: detect system os, we need this infomation to define our own package
# managerment
JOB=''
SCRIPT=$1
VERBOSE=""
PIPELINE="$(dirname "$0")"

source $PIPELINE/Libraries/Logcat.sh
source $PIPELINE/Libraries/Package.sh

shift

function cleanup() {
	if [ -f $PIPELINE/cleanup.pid ]; then
		cat $PIPELINE/cleanup.pid | while read JOB; do
			if [ -f /tmp/enqueue-$(whoami)/$JOB ]; then
				rm -fr /tmp/enqueue-$(whoami)/$JOB

				if [[ ${#VERBOSE} -gt 0 ]]; then
					if $VERBOSE $SCRIPT clean --verbose $@; then
						break
					fi
				else
					if $VERBOSE $SCRIPT clean $@; then
						break
					fi
				fi
			fi
		done

		rm -fr $PIPELINE/cleanup.pid
	fi
}

if [ -f $HOME/environment.sh ]; then
	source $HOME/environment.sh
fi

while [ $# -gt 0 ]; do
	case $1 in
		--verbose)	VERBOSE="bash -x"; set -x;;
		--cleanup)	cleanup; exit $?;;
		(--) 		shift; break;;
		(-*) 		error "unrecognized option $1";;
		(*) 		break;;
	esac
	shift
done

while [ 1 ]; do
	# @STEP 1: init a new job
	mkdir -p /tmp/enqueue-$(whoami)

	while [ 1 ]; do
		JOB=$(date +%s)

		if [ -f /tmp/enqueue-$(whoami)/$JOB ]; then
			sleep 1
		elif touch /tmp/enqueue-$(whoami)/$JOB; then
			break
		fi
	done

	# @STEP 2: register this job to be clean-up if the job is cancel
	echo $JOB >> $PIPELINE/cleanup.pid

	# @STEP 3: probe if we can plan a new job
	while [ 1 ]; do
		# @NOTE: by default, subcommand probe will check and plan the first
		# variable. I assume there is no racing here because we can't create the
		# same variable at the same time.

		if [[ ${#VERBOSE} -gt 0 ]]; then
			if $VERBOSE $SCRIPT probe --verbose $@; then
				break
			else
				sleep 3
			fi
		else
			if $VERBOSE $SCRIPT probe $@; then
				break
			else
				sleep 3
			fi
		fi
	done

	# @STEP 4: remove the job since we nealy have this job on-board and i don't think
	# we fail here
	rm -fr $JOB

	# @STEP 5: plan this job to be on-board. At this step, only one job jumps here
	# and we are planing this job to be handled.
	if [[ ${#VERBOSE} -gt 0 ]]; then
		if $VERBOSE $SCRIPT plan --verbose $@; then
			# @STEP 6: the job has already been on-board and we should run it now to collect
			# log from the ci

			$VERBOSE $SCRIPT run --verbose $@
			CODE=$?
		elif $VERBOSE $SCRIPT check --verbose; then
			continue
		else
			break
		fi
	else
		if $VERBOSE $SCRIPT plan $@; then
			# @STEP 6: the job has already been on-board and we should run it now to collect
			# log from the ci

			$VERBOSE $SCRIPT run $@
			CODE=$?
		elif $VERBOSE $SCRIPT check; then
			continue
		else
			break
		fi
	fi

	break
done

exit $CODE
