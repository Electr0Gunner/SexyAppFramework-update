#ifndef __DISCORD_HPP__
#define __DISCORD_HPP__
#ifdef _WIN32
#pragma once
#endif

#ifdef _FEATURE_DISCORD_RPC

#include <discord-rpc.hpp>


namespace PopWork
{ 
    struct RPCData
    {
        std::string mState;
        std::string mDetails;
        std::string mSmallImageName;
        std::string mLargeImageName;
    };

    class DiscordRPC
    {
    private:

        std::string mAppID;
        RPCData mRPCData;

        bool InitRPC();
    public:
        DiscordRPC(const char* theAppID = "1369297870456488057");
        ~DiscordRPC();
        void UpdateRPC();
        int64_t mStartTime;
        bool mSendRPC;
        bool mHasInitialized;
    };

} // namespace PopWork


#endif

#endif