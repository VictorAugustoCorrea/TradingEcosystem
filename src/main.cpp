#include "low-latency-components/thread_utils.h"
#include "low-latency-components/mem_pool.h"
#include "low-latency-components/lock_free_queue.h"
#include "low-latency-components/logging.h"
#include "low-latency-components/tcp_server.h"
#include "exchange/matcher/matching_engine.h"
#include <csignal>

using namespace Common;
using namespace std::literals::chrono_literals;

Logger* logger = nullptr;
Exchange::MatchingEngine* matching_engine = nullptr;

/** test threads */
auto dummyFunction(const int a, const int b, const bool sleep)
{
    std::cout << "Dummy function(" << a << "," << b << ")" << std::endl;
    std::cout << "Dummy output = " << a * b << std::endl;

    if (sleep)
    {
        std::cout << "dummyFunction sleeping ..." << std::endl;
        std::this_thread::sleep_for(5s);
    }
    std::cout << "dummyFunction done. " << std::endl;
}

struct MyStruct
{
    int d_[3];
};

/** test lock free queue */
auto consumeFunction(LFQueue<MyStruct>* lfq)
{
    std::this_thread::sleep_for(5s);

    while (lfq -> size())
    {
        const auto d = lfq -> getNextToRead();
        lfq -> updateWriteIndex();

        std::cout << "consumeFunction read elem: "
            << d -> d_[0] << ", "
            << d -> d_[1] << ", "
            << d -> d_[2] << " lfq-size: " << lfq -> size() << std::endl;

        std::this_thread::sleep_for(1s);
    }
    std::cout << "consumeFunction exiting." << std::endl;
}

/** Signal handler */
void signal_handler(int) {
    std::this_thread::sleep_for(10s);

    delete logger;
    logger = nullptr;

    delete matching_engine;
    matching_engine = nullptr;

    std::this_thread::sleep_for(10s);
    exit(EXIT_SUCCESS);
}

int main(int, char **)
{
    // const auto t1 = createAndStartThread(-1, "dummyFunction1", dummyFunction, 10, 30, false);
    // const auto t2 = createAndStartThread( 1, "dummyFunction2", dummyFunction, 20, 51, true );
    //
    // std::cout << "main waiting for threads to be done." << std::endl;
    // t1 -> join();
    // t2 -> join();
    //
    // MemPool<double> prim_pool(5);
    // MemPool<MyStruct> struct_pool(5);
    //
    // for (auto i = 0; i < 5; i++)
    // {
    //     const auto p_ret = prim_pool.allocate(i);
    //     const auto s_ret = struct_pool.allocate(MyStruct{i, i + 1, i + 2});
    //
    //     std::cout << "prim elem: " << *p_ret << " allocated at: " << p_ret << std::endl;
    //     std::cout << "struct elem: "
    //         << s_ret ->d_[0] << ", "
    //         << s_ret ->d_[1] << ", "
    //         << s_ret ->d_[2] << " allocated at: "
    //         << s_ret << std::endl;
    //     if (i % 5 == 0)
    //     {
    //         std::cout << "deallocating prim elem: " << *p_ret << " from: " << p_ret << std::endl;
    //         std::cout << "deallocating struct elem: "
    //             << s_ret ->d_[0] << ", "
    //             << s_ret ->d_[1] << ", "
    //             << s_ret ->d_[2] << " from: " << s_ret << std::endl;
    //
    //         prim_pool.deallocate(p_ret);
    //         struct_pool.deallocate(s_ret);
    //     }
    // }
    //
    // LFQueue<MyStruct> lfq(20);
    //
    // const auto ct = createAndStartThread(-1, "", consumeFunction, &lfq);
    //
    // for ( auto i = 0; i < 50; i++)
    // {
    //     const MyStruct d{1, i * 10, i * 100};
    //     *lfq.getNextToWriteTo() = d;
    //     lfq.updateWriteIndex();
    //
    //     std::cout << "main constructed elem: "
    //         << d.d_[0] << ", "
    //         << d.d_[1] << ", "
    //         << d.d_[2] << " lfq-size"
    //         << lfq.size() << std::endl;
    //
    //     std::this_thread::sleep_for(1s);
    // }
    // ct -> join();
    // std::cout << "main exiting." << std::endl;

    // constexpr char c = 'd';
    // constexpr int i  = 3;
    // constexpr unsigned long ul = 65;
    // constexpr float f = 3.4;
    // constexpr double d = 34.56;
    // const auto s = "test C-string";
    // const std::string ss = "Test string";
    //
    // Logger logger("logging_example.log");
    //
    // logger.log("logging a char: %, an int: % and an unsigned: % \n", c, i, ul);
    // logger.log("Logging a float: % and a double: % \n", f, d);
    // logger.log("Logging a c-string: % \n", s);
    // logger.log("Logging a string: % \n", ss);

    // /** Setting socket parameters*/
    // std::string time_str_;
    // const std::string iface = "lo";
    // const std::string ip = "127.0.0.1";
    // constexpr int port = 12345;
    //
    // Logger logger_("socket_example.log");
    // auto tcpServerRecvCallback = [&](TCPSocket *socket, const Nanos rx_time) noexcept {
    //     logger_.log("TCPServer::defaultRecvCallback() socket: %, len: %, rx: % \n",
    //         socket -> fd_,
    //         socket -> next_rcv_valid_index_,
    //         rx_time);
    //     const std::string reply = "TCPServer received msg: "
    //     + std::string(socket -> rcv_buffer_, socket -> next_rcv_valid_index_ = 0);
    //
    //     socket -> send(reply.data(), reply.length());
    // };
    //
    // auto tcpServerRecvFinishedCallback = [&]() noexcept {
    //     logger_.log("TCPServer::defaultRecvFinishedCallback() \n");
    // };
    //
    // auto tcpClientRecvCallback = [&](TCPSocket *socket, const Nanos rx_time) noexcept {
    //     const auto recv_msg = std::string(socket -> rcv_buffer_, socket -> next_rcv_valid_index_);
    //     socket -> next_rcv_valid_index_ = 0;
    //
    //     logger_.log("TCPSocket::defaultRecvCallback()  socket: %, len: %, rx: %, msg: %. \n",
    //         socket -> fd_,
    //         socket -> next_rcv_valid_index_,
    //         rx_time,
    //         recv_msg);
    // };
    //
    // logger_.log("Creating TCPServer on iface: %, port: % \n", iface, port);
    //
    // TCPServer server(logger_);
    // server.recv_callback_ = tcpServerRecvCallback;
    // server.recv_finished_callback_ = tcpServerRecvFinishedCallback;
    // server.listen(iface, port);
    //
    // std::vector<TCPSocket *> clients(5);
    //
    // for (size_t i  = 0; i < clients.size(); ++i) {
    //     clients[i] = new TCPSocket(logger_);
    //     clients[i] -> recv_callback_ = tcpClientRecvCallback;
    //
    //     logger_.log("Connecting TCPClient-[%] on ip: %, iface: %, port: %. \n",
    //         i, ip, iface, port);
    //     clients[i] -> connect(ip, iface, port, false);
    //     server.poll();
    // }
    //
    // for (auto itr = 0; itr < clients.size(); ++itr) {
    //     for (size_t i = 0; i < clients.size(); ++i) {
    //         const std::string client_msg = "CLIENT-[ "
    //         + std::to_string(i) + " ] : Sending: "
    //         + std::to_string(itr * 100 + i);
    //
    //         logger_.log("Sending TCPClient - [%] %. \n", i, client_msg);
    //         clients[i] -> send(client_msg.data(), client_msg.length());
    //         clients[i] -> sendAndRecv();
    //     }
    // }
    //
    // for (auto itr = 0; itr < 5; ++itr) {
    //     for (const auto &client: clients)
    //         client -> sendAndRecv();
    //
    //     server.poll();
    //     server.sendAndRecv();
    //     std::this_thread::sleep_for(500ms);
    // }

    logger = new Logger("exchange_main.log");
    std::signal(SIGINT, signal_handler);

    constexpr int sleep_time = 100 * 1000;

    Exchange::ClientRequestLFQueue client_requests(ME_MAX_CLIENT_UPDATES);
    Exchange::MEClientResponseLFQueue client_responses(ME_MAX_CLIENT_UPDATES);
    Exchange::MEMarketUpdateLFQueue market_updates(ME_MAX_CLIENT_UPDATES);

    std::string time_str;
    logger -> log("%:% %() % Starting Matching Engine ... \n",
        __FILE__, __LINE__, __func__,
        getCurrentTimeStr(&time_str));
    matching_engine = new Exchange::MatchingEngine(&client_requests, &client_responses, &market_updates);
    matching_engine -> start();

    while (true){
        logger -> log("%:% %() % Sleeping for a few milliseconds ...\n",
            __FILE__, __LINE__, __func__,
            getCurrentTimeStr(&time_str));
        usleep(sleep_time);
    }

    return 0;
}