//------------------------------------------------------------------------------
// Example header file, creating user mappings to Lua
//------------------------------------------------------------------------------

#ifndef __QUSER_SCORELOOP_H
#define __QUSER_SCORELOOP_H

#include "coresocial\h\scoreloop\scoreloopcore.h"
#include <string>
#include <map>

// tolua_begin
namespace quickuser {
// tolua_end

//------------------------------------------------------------------------------
// Scoreloop
//------------------------------------------------------------------------------
class QScoreloop { // tolua_export
public:
    // BOUND, PRIVATE
	// tolua_begin
	std::string __tostring() { return "<>"; }
    void* __serialize() { return NULL; }
    QScoreloop();
    ~QScoreloop();

    // BOUND, PUBLIC
	void init(const char* gameId, const char* gameSecret, const char* gameVersion, const char* currency, const char* languages);
	void terminate();
	void cancel();
	void refresh();
	void submitResult(unsigned int result, unsigned int minorResult, unsigned int level);
	// tolua_end

    // UNBOUND
private:
	// Ranking controller that is used to obtain the
	// users rank from the server.
	SC_RankingController_h m_rankingController;
	void rankingResult(SC_Error_t completionStatus);
	static void rankingControllerCallback(void *userData, SC_Error_t completionStatus);

	// Score controller that is used to submit the
	// users score to the server.
	SC_ScoreController_h m_scoreController;
	void scoreResult(SC_Error_t completionStatus);
	static void scoreControllerCallbackSubmit(void *userData, SC_Error_t completionStatus);

	// Client data
	SC_Client_h m_client;
	SC_InitData_t m_initData;

	// Send lua events
	void sendLuaEvent(const char* phase, bool success);
	void sendLuaEvent(const char* phase, bool success, std::map<std::string, unsigned int >& dataMap);

	// Refresh the users rank
	void refreshRank(bool initializing);

	// Destroy all resources
	void destroy();

	// Locally maintained scores
	unsigned int m_result;
	unsigned int m_minorResult;
	unsigned int m_level;
	unsigned int m_rank;
	bool m_available;
	bool m_initialized;
	std::string m_login;
}; // tolua_export

// tolua_begin
}
// tolua_end

#endif // __QUSER_SCORELOOP_H
