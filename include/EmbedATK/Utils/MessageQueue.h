#pragma once

#include "EmbedATK/OSAL/OSAL.h"

namespace Utils {

    template<IsStaticPolymorphic Queue, typename T, size_t Size>
    requires (std::is_base_of_v<IPolymorphic<OSAL::MessageQueue>, Queue>)
    struct StaticMessageQueue
    {
        inline static constexpr size_t SIZE = Size;

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
    concept IsStaticMessageQueue = is_static_message_queue_v<T>;

    template<IsStaticMessageQueue Queue>
    static constexpr void setupStaticMessageQueue(Queue& queue)
    {
        OSAL::createMessageQueue(queue.queue, queue.queueStore, queue.queuePool);
    }

    template<IsStaticMessageQueue Queue, typename T>
    requires std::is_move_constructible_v<T>
    static constexpr bool pushStaticMessageQueue(Queue& queue, T&& msg)
    {
        if (!queue.dataPool.hasSpace()) return false;
        T* ptr = queue.dataPool.template construct<T>(std::move(msg));
        return queue.queue.get()->push(OSAL::MessageQueue::MsgType(std::in_place_type<T*>, ptr));
    }

    template<IsStaticMessageQueue Queue, typename T>
    requires std::is_copy_constructible_v<T>
    static constexpr bool pushStaticMessageQueue(Queue& queue, const T& msg)
    {
        if (!queue.dataPool.hasSpace()) return false;
        T* ptr = queue.dataPool.template construct<T>(msg);
        return queue.queue.get()->push(OSAL::MessageQueue::MsgType(std::in_place_type<T*>, ptr));
    }

    template<IsStaticMessageQueue Queue, typename T>
    requires std::is_move_constructible_v<T>
    static constexpr bool pushManyStaticMessageQueue(Queue& queue, IQueue<T>&& data)
    {
        for (T& msg : data) {
            if (!queue.dataPool.hasSpace()) return false;
            T* ptr = queue.dataPool.template construct<T>(std::move(msg));
            if (!queue.queue.get()->push(OSAL::MessageQueue::MsgType(std::in_place_type<T*>, ptr))) {
                return false;
            }
        }
        data.clear();
        return true;
    }

    template<IsStaticMessageQueue Queue, typename T>
    requires std::is_copy_constructible_v<T>
    static constexpr bool pushManyStaticMessageQueue(Queue& queue, const IQueue<T>& data)
    {
        for (const T& msg : data) {
            if (!queue.dataPool.hasSpace()) return false;
            T* ptr = queue.dataPool.template construct<T>(msg);
            if (!queue.queue.get()->push(OSAL::MessageQueue::MsgType(std::in_place_type<T*>, ptr))) {
                return false;
            }
        }
        return true;
    }

    template<typename T, IsStaticMessageQueue Queue, typename ...Args>
    static constexpr bool emplaceStaticMessageQueue(Queue& queue, Args&&... args)
    {
        if (!queue.dataPool.hasSpace()) return false;
        T* ptr = queue.dataPool.template construct<T>(std::forward<Args>(args)...);
        return queue.queue.get()->push(OSAL::MessageQueue::MsgType(std::in_place_type<T*>, ptr));
    }

    template<typename T, IsStaticMessageQueue Queue>
    static constexpr std::optional<T> popStaticMessageQueue(Queue& queue)
    {
        std::optional<OSAL::MessageQueue::MsgType> ptr = queue.queue.get()->pop();
        if (!ptr.has_value()) {
            return std::nullopt;
        }
        
        std::optional<T> msg = std::move(*ptr->asUnchecked<T*>());
        queue.dataPool.destroy(ptr->asUnchecked<T*>());
        return msg;
    }

    template<IsStaticMessageQueue Queue, typename T, size_t N>
    static constexpr bool popAvailStaticMessageQueue(Queue& queue, StaticQueue<T, N>& data)
    {
        StaticQueue<OSAL::MessageQueue::MsgType, N> localQueue;

        if (!queue.queue.get()->popAvail(localQueue)) {
            return false;
        }

        data.clear();
        for (OSAL::MessageQueue::MsgType& ptr : localQueue) {
            data.push(std::move(*ptr.asUnchecked<T*>()));
            queue.dataPool.destroy(ptr.asUnchecked<T*>());
        }
        return true;
    }

    template<IsStaticMessageQueue Queue, typename T, size_t N>
    static constexpr bool tryPopAvailStaticMessageQueue(Queue& queue, StaticQueue<T, N>& data)
    {
        StaticQueue<OSAL::MessageQueue::MsgType, N> localQueue;

        if (!queue.queue.get()->tryPopAvail(localQueue)) {
            return false;
        }

        data.clear();
        for (OSAL::MessageQueue::MsgType& ptr : localQueue) {
            data.push(std::move(*ptr.asUnchecked<T*>()));
            queue.dataPool.destroy(ptr.asUnchecked<T*>());
        }
        return true;
    }

}