#include "AsyncQueue.hpp"

using namespace Viewer::UI::Adapters;

void AsyncQueue::initialize(QQuickVtkItem* target) noexcept
{
    target_ = target;
}

void AsyncQueue::execute(Task task) noexcept
{
    Q_ASSERT(target_ != nullptr);
    dispatch_async(std::move(task));
}

void AsyncQueue::update() noexcept
{
    target_->forceUpdate();
}

void AsyncQueue::dispatch_async(std::function<void()> callback)
{
    auto wrapped = [task =
                        std::move(callback)](vtkRenderWindow*, const QQuickVtkItem::vtkUserData&) {
        task();
    };
    target_->dispatch_async(wrapped);
}