#ifndef BASE_TASK_H
#define BASE_TASK_H

#define MINIMUM_STACK_SIZE 4096
#define MIDSIZE_STACK_SIZE 8192
#define MAX_STACK_SIZE     16384

class BaseTask {
    public:
        virtual void startTask();
        virtual bool suspendTask();
        virtual bool resumeTask();
};

#endif