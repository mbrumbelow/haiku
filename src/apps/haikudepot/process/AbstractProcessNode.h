/*
 * Copyright 2018-2022, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef ABSTRACT_PROCESS_NODE_H
#define ABSTRACT_PROCESS_NODE_H


#include <AutoLocker.h>
#include <Locker.h>
#include <ObjectList.h>
#include <OS.h>


class AbstractProcess;
class ProcessListener;


/*! This class is designed to be used by the ProcessCoordinator class.  The
    purpose of the class is to hold a process and also any dependent processes
    of this one.  This effectively creates a dependency tree of processes.
*/

class AbstractProcessNode {
public:
								AbstractProcessNode(AbstractProcess* process);
	virtual						~AbstractProcessNode();

			AbstractProcess*	Process() const;
	virtual	status_t			Start() = 0;
	virtual	status_t			RequestStop() = 0;
	virtual	bool				IsRunning();

			void				AddPredecessor(AbstractProcessNode* node);
			int32				CountPredecessors() const;
			AbstractProcessNode*
								PredecessorAt(int32 index) const;
			bool				AllPredecessorsComplete() const;

			int32				CountSuccessors() const;
			AbstractProcessNode*
								SuccessorAt(int32 index) const;

	virtual	void				SetListener(ProcessListener* listener);

protected:
			status_t			_SpinUntilProcessState(
									uint32 desiredStatesMask,
									int32 timeoutSeconds);

protected:
			BLocker				fLock;
			ProcessListener*	fListener;

private:
			void				_AddSuccessor(AbstractProcessNode* node);

			AbstractProcess*	fProcess;
			BObjectList<AbstractProcessNode>
								fPredecessorNodes;
			BObjectList<AbstractProcessNode>
								fSuccessorNodes;
};


#endif // ABSTRACT_PROCESS_NODE_H
