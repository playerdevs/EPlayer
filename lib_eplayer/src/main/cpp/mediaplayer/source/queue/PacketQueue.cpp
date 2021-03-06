
#include "PacketQueue.h"

PacketQueue::PacketQueue() {
    abort_request = 0;
    first_pkt = NULL;
    last_pkt = NULL;
    nb_packets = 0;
    size = 0;
    duration = 0;
}

PacketQueue::~PacketQueue() {
    abort();
    flush();
}

/**
 * 入队数据包
 * @param pkt
 * @return
 */
int PacketQueue::put(AVPacket *pkt) {
    // 一个队列元素，包括当前pkt和下一个pkt的指针
    PacketList *pkt1;

    if (abort_request) {
        return -1;
    }
    // 分配结构体内存
    pkt1 = (PacketList *) av_malloc(sizeof(PacketList));
    if (!pkt1) {
        return -1;
    }
    // 赋值
    pkt1->pkt = *pkt;
    pkt1->next = NULL;

    if (!last_pkt) { // 如果最后一个元素为null，代表当前是添加第一个pkt
        first_pkt = pkt1;
    } else {
        // 连接到上一个元素的后面
        last_pkt->next = pkt1; //最后元素的下一个元素的指针指向pkt1
    }
    // 最后一个元素赋值为pkt1
    last_pkt = pkt1;
    nb_packets++; // 数据大小
    size += pkt1->pkt.size + sizeof(*pkt1);
    duration += pkt1->pkt.duration;
    return 0;
}

/**
 * 入队数据包
 * @param pkt
 * @return
 */
int PacketQueue::pushPacket(AVPacket *pkt) {
    int ret;
    // 线程安全
    mMutex.lock();
    ret = put(pkt);
    // 通知阻塞的线程，比如取数据线程被阻塞的时候，这里可以通知它有数据可以取了，解除阻塞
    mCondition.signal();
    mMutex.unlock();

    if (ret < 0) {
        av_packet_unref(pkt);
    }

    return ret;
}

int PacketQueue::pushNullPacket(int stream_index) {
    AVPacket pkt1, *pkt = &pkt1;
    av_init_packet(pkt);
    pkt->data = NULL;
    pkt->size = 0;
    pkt->stream_index = stream_index;
    return pushPacket(pkt);
}

/**
 * 清空队列
 */
void PacketQueue::flush() {
    PacketList *pkt, *pkt1;

    mMutex.lock();
    // pkt = first_pkt是最开始初始化条件，判断pkt是否为真，pkt = pkt1是执行括号后更新的条件
    for (pkt = first_pkt; pkt; pkt = pkt1) {
        pkt1 = pkt->next;
        av_packet_unref(&pkt->pkt);
        av_freep(&pkt);
    }
    last_pkt = NULL;
    first_pkt = NULL;
    nb_packets = 0;
    size = 0;
    duration = 0;
    mCondition.signal();
    mMutex.unlock();
}

/**
 * 队列终止
 */
void PacketQueue::abort() {
    mMutex.lock();
    abort_request = 1;
    mCondition.signal();
    mMutex.unlock();
}

/**
 * 队列开始
 */
void PacketQueue::start() {
    mMutex.lock();
    abort_request = 0;
    mCondition.signal();
    mMutex.unlock();
}

/**
 * 取出数据包
 * @param pkt
 * @return
 */
int PacketQueue::getPacket(AVPacket *pkt) {
    return getPacket(pkt, 1);
}

/**
 * 取出数据包
 * @param pkt
 * @param block 是否阻塞
 * @return
 */
int PacketQueue::getPacket(AVPacket *pkt, int block) {
    PacketList *pkt1;
    int ret;

    mMutex.lock();

    for (;;) {
        //如果禁止取数据，则break
        if (abort_request) {
            ret = -1;
            break;
        }
        // 取最前面的那个元素
        pkt1 = first_pkt;
        if (pkt1) { // 首元素是否为null，不为null代表队列有元素
            first_pkt = pkt1->next; // 取出first_pkt，所以首元素变为下一个
            if (!first_pkt) { // 没有元素了
                last_pkt = NULL;
            }
            nb_packets--;
            size -= pkt1->pkt.size + sizeof(*pkt1);
            duration -= pkt1->pkt.duration;
            *pkt = pkt1->pkt;
            av_free(pkt1); // 释放
            ret = 1;
            break;
        } else if (!block) { // 不阻塞
            ret = 0;
            break;
        } else { // 阻塞
            mCondition.wait(mMutex);
        }
    }
    mMutex.unlock();
    return ret;
}

int PacketQueue::getPacketSize() {
    Mutex::Autolock lock(mMutex);
    return nb_packets;
}

int PacketQueue::getSize() {
    return size;
}

int64_t PacketQueue::getDuration() {
    return duration;
}

int PacketQueue::isAbort() {
    return abort_request;
}
