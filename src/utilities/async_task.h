#ifndef ASYNC_TASK_H
#define ASYNC_TASK_H

#include <functional>
#include <memory>
#include <QObject>
#include <QPointer>
#include <QTimer>

/*
 * TODO:
 * + on thread-safe
*/



template <class ResultType>
class AbstractAsyncTask
{
public:
    explicit AbstractAsyncTask() {}
    virtual ~AbstractAsyncTask() {}

    virtual void start() = 0;

    ResultType result;

    void done() {}
};


template <class ResultType>
class AsyncTask : public AbstractAsyncTask<ResultType>
{
public:
    explicit AsyncTask() : AbstractAsyncTask<ResultType>() {}

    std::function<void ()> mFunc;

    void start() override {
        mFunc();
    }
};

template <class ResultType>
using AsyncTaskSPtr = std::shared_ptr< AsyncTask<ResultType> >;

template <class ResultType>
using AsyncTaskWPtr = std::weak_ptr< AsyncTask<ResultType> >;

template <class ResultType>
std::pair<AsyncTaskSPtr<ResultType>, AsyncTaskWPtr<ResultType> > createAsyncTaskPointers() {
    auto task = std::make_shared<AsyncTask<int>>();
    return std::make_pair(task, AsyncTaskWPtr<int>(task));
}

//class AsyncTaskSeqeunce : public IAsyncTask {
//public:
//    explicit AsyncTaskSeqeunce(QObject *parent = nullptr);

//    int result;
//    void start() override;

//    //
//    void addTask(IAsyncTask *task);

//private:

//};



//


#endif // ASYNC_TASK_H
