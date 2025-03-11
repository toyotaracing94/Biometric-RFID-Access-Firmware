#ifndef BASE_TASK_H
#define BASE_TASK_H

class BaseTask {
    public:
        virtual void createTask();
        virtual void suspendTask();
        virtual void resumeTask();
};

#endif