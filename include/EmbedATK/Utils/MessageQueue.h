#pragma once

#include "EmbedATK/OSAL/OSAL.h"

namespace Utils {

    template<IsStaticPolymorphic Queue, typename T, size_t Size>
    requires (std::is_base_of_v<IPolymorphic<OSAL::MessageQueue>, Queue>)
    struct StaticMessageQueue
    {
        StaticBlockPool<Size, allocData<OSAL::MessageQueue::MsgType>()> queuePool;
        StaticObjectStore<OSAL::MessageQueue::MsgType*, Size> queueStore;
        Queue queue;

        StaticBlockPool<Size, allocData<T>()> dataPool;
    };

    template <typename T>
    struct is_static_message_queue : std::false_type {};

    template<IsStaticPolymorphic Queue, typename T, size_t Size>
    struct is_static_message_queue<Utils::StaticMessageQueue<Queue, T, Size>> : std::true_type {};

    template <typename T>
    inline constexpr bool is_static_message_queue_v = is_static_message_queue<T>::value;

    template <typename T>
    concept IsStaticMessageQueue = requires { 
        typename T;
        { T::queuePool };
        { T::queueStore };
        { T::queue };
        { T::dataPool };

        requires is_static_message_queue_v<T>;
    };

    template<IsStaticMessageQueue Queue>
    static constexpr void setupStaticMessageQueue(Queue& queue)
    {
        OSAL::createMessageQueue(queue.queue, queue.queueStore, queue.queuePool);
    }

    template<IsStaticMessageQueue Queue, typename T>
    requires std::is_move_constructible_v<T>
    static constexpr void pushStaticMessageQueue(Queue& queue, T&& msg)
    {
        T* ptr = queue.dataPool.template construct<T>(std::move(msg));
        queue.queue.get()->push(OSAL::MessageQueue::MsgType(std::in_place_type<T*>, ptr));
    }

    template<IsStaticMessageQueue Queue, typename T>
    requires std::is_copy_constructible_v<T>
    static constexpr void pushStaticMessageQueue(Queue& queue, const T& msg)
    {
        T* ptr = queue.dataPool.template construct<T>(msg);
        queue.queue.get()->push(OSAL::MessageQueue::MsgType(std::in_place_type<T*>, ptr));
    }

    template<typename T, IsStaticMessageQueue Queue, typename ...Args>
    static constexpr void emplaceStaticMessageQueue(Queue& queue, Args&&... args)
    {
        T* ptr = queue.dataPool.template construct<T>(std::forward<Args>(args)...);
        queue.queue.get()->push(OSAL::MessageQueue::MsgType(std::in_place_type<T*>, ptr));
    }

    template<IsStaticMessageQueue Queue, typename T>
    static constexpr std::optional<T> popStaticMessageQueue(Queue& queue)
    {
        std::optional<OSAL::MessageQueue::MsgType> ptr = queue.queue.get()->pop();
        if (!ptr.has_value()) {
            return std::nullopt;
        }
        
        std::optional<T> msg = std::move(*ptr.value() );
        queue.dataPool.destroy(ptr);
        return msg;
    }

    template<IsStaticMessageQueue Queue, typename T>
    static constexpr bool popAvailStaticMessageQueue(Queue& queue, IQueue<T>& data)
    {
        StaticQueue<OSAL::MessageQueue::MsgType, queue.queueStore.size()> localQueue;

        if (!queue.queue.get()->popAvail(localQueue)) {
            return false;
        }

        data.clear();
        while (!localQueue.empty()) {
            OSAL::MessageQueue::MsgType ptr = queue.queue.get()->pop().value();
            data.push(std::move(*ptr));
            queue.dataPool.destroy(ptr);
        }

        return true;
    }

}