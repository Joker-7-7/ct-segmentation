#pragma once

#include "Backend/ThirdParty/QQuickVtkItem.h"

#include <functional>

#include <QtClassHelperMacros>

namespace Viewer::UI::Adapters
{
    /**
     * Intermidiate wrapper for delegating async VTK callbacks to #QQuickVtkItem::dispatch_async.
     * Provides common communication point throught #Task and #execute.
     * Abstracts the process of pushing async callbacks into VTK execution queue.
     */
    class AsyncQueue
    {
        Q_DISABLE_COPY_MOVE(AsyncQueue)

    public:
        /**
         * Initializes underlying #target_ reference to delegate events to it.
         *
         * @param target VTK item which will handle events.
         */
        void initialize(QQuickVtkItem* target) noexcept;

    protected:
        using Task = std::function<void()>;

        /**
         * Ctor.
         */
        AsyncQueue() noexcept = default;

        /**
         * Dtor.
         */
        virtual ~AsyncQueue() noexcept = default;

        /**
         * Adds new callback task to an execution queue.
         *
         * @param task Callback to be executed.
         */
        void execute(Task task) noexcept;

        /**
         * Forces the target renderer to update instantly.
         *
         * @see #QQuickVtkItem::forceUpdate()
         */
        void update() noexcept;

        void dispatch_async(std::function<void()> callback);
        /**
         * Target renderer which accepts async events.
         */
        QQuickVtkItem* target_{ nullptr };
    };
}
