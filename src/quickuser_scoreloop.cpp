#include "quickuser_scoreloop.h"
#include "QLuaHelpers.h"

using namespace quick;
using namespace quickuser;

// Global event phases
const char* g_resultSubmitEventPhase = "userResultSubmit";
const char* g_rankRequestEventPhase = "userRankRequest";

// Global data
const int g_mode = 0;

// All events have the following data format:
// event is 'scoreloop'
// phase is either userResultSubmit or userRankRequest
// success is true or false
//
// userResultSubmit has no additional data for success or failures.
// userRankRequest on success will provide the user's player, rank, result, minorResult, and level.
//
// On init() call these events should follow:
// userRankRequest
//
// On refresh() call these events should follow:
// userRankRequest
//
// On submiteResult() call these events should follow:
// userResultSubmit -> userRankRequest
//
// Note that all ->'s will only proceed to the next event
// if the preceeding event is successful, otherwise the
// event after the -> should not be expected.

#pragma region Constructor/Destructor
// Constructor
QScoreloop::QScoreloop() :
m_available(false),
m_initialized(false),
m_scoreController(NULL),
m_rankingController(NULL),
m_client(NULL),
m_result(0),
m_minorResult(0),
m_level(0),
m_rank(0),
m_login("")
{
	// Initialize scoreloop data.
	SC_InitData_Init(&m_initData);
}

// Destructor
QScoreloop::~QScoreloop()
{
}
#pragma endregion

#pragma region LUA API (Quick)
// Initialize the class
void QScoreloop::init(const char* gameId, const char* gameSecret, const char* gameVersion, const char* currency, const char* languages)
{
	QTrace("Scoreloop: (DEBUG) init called");

	// Guard check on initialized
	if(m_initialized)
	{
		return;
	}

	// ERROR code returned
	SC_Error_t retCode;

	// Create the client
	if(m_client == NULL)
	{
		retCode = SC_Client_New(&m_client, &m_initData, gameId, gameSecret, gameVersion, currency, languages);
		if(retCode != SC_OK)
		{
			QTrace("Scoreloop: (ERROR) failed to initialize scoreloop client! %s", SC_MapErrorToStr(retCode));
			m_client = NULL;
			destroy();
			return;
		}
	}

	// Create the score controller
	if (m_scoreController == NULL)
	{
		retCode = SC_Client_CreateScoreController(m_client, &m_scoreController, scoreControllerCallbackSubmit, this);
		if(retCode != SC_OK)
		{
			QTrace("Scoreloop: (ERROR) failed to initialize scoreloop score controller! %s", SC_MapErrorToStr(retCode));
			m_scoreController = NULL;
			destroy();
			return;
		}
	}
	
	// Create the ranking controller
	if (m_rankingController == NULL)
	{
		retCode = SC_Client_CreateRankingController(m_client, &m_rankingController, rankingControllerCallback, this);
		if(retCode != SC_OK)
		{
			QTrace("Scoreloop: (ERROR) Scoreloop: failed to initialize scoreloop rank controller! %s", SC_MapErrorToStr(retCode));
			m_rankingController = NULL;
			destroy();
			return;
		}
	}
	
	// Set the scores controll search list to all
	if(SC_RankingController_SetSearchList(m_rankingController, SC_SCORES_SEARCH_LIST_ALL) != SC_OK)
	{
		QTrace("Scoreloop: (ERROR) failed to set scoreloop search global list!");
		destroy();
		return;
	}

	// Indicate that we intialized
	m_initialized = true;

	// Load the users ranking and user
	// information from the server.
	refreshRank(true);
}

// Terminates scoreloop
void QScoreloop::terminate()
{
	QTrace("Scoreloop: (DEBUG) terminate called");

	// Guard check on initialized
	if(!m_initialized)
	{
		QTrace("Scoreloop: (WARN) terminate called when not initialized!");
		return;
	}

	// Destroy our resources
	destroy();

	// Clear state
	m_available = false;
	m_initialized = false;
}

// Cancel open requests
void QScoreloop::cancel()
{
	QTrace("Scoreloop: (DEBUG) cancel called");

	if(m_initialized && m_available)
	{
		// Cancel the score controller
		if(m_scoreController)
		{
			SC_ScoreController_Cancel(m_scoreController);
		}

		// Cancel the ranking controller
		if (m_rankingController)
		{
			SC_RankingController_Cancel(m_rankingController);
		}
	}
	else
	{
		QTrace("Scoreloop: (WARN) cancel called [%s, %s]",
			m_initialized ? "initialized" : "not initialized",
			m_available ? "available" : "unavailable");
	}
}

// Refresh our state
void QScoreloop::refresh()
{
	QTrace("Scoreloop: (DEBUG) refresh called");
	refreshRank(false);
}

// Set the users scores
void QScoreloop::submitResult(unsigned int result, unsigned int minorResult, unsigned int level)
{
	QTrace("Scoreloop: (DEBUG) submitResult called with result=%d minorResult=%d level=%d", result, minorResult, level);

	// Guard check on available
	if(!m_available || !m_initialized)
	{
		QTrace("Scoreloop: (WARN) submitResult called [%s, %s]",
			m_initialized ? "initialized" : "not initialized",
			m_available ? "available" : "unavailable");
		scoreResult(SC_INVALID_STATE);
		return;
	}

	// If they have a new score
	if((result > m_result) ||
		(result == m_result && minorResult > m_minorResult))
	{
		// The return code
		SC_Error_t retCode;

		// Cancel any in progress request.
		retCode = SC_ScoreController_Cancel(m_scoreController);
		if(retCode != SC_OK)
		{
			QTrace("Scoreloop: (ERROR) cancel score controller failed! %s", SC_MapErrorToStr(retCode));
		}

		// Update our local scores
		m_result = result;
		m_level = level;
		m_minorResult = minorResult;

		// Create an SC score
		SC_Score_h scScore;
		retCode = SC_Client_CreateScore(m_client, &scScore);
		if (retCode == SC_OK)
		{
			// Set the score properties
			SC_Score_SetResult(scScore, m_result);
			SC_Score_SetMode(scScore, g_mode);
			SC_Score_SetLevel(scScore, level);
			SC_Score_SetMinorResult(scScore, minorResult);

			// Submit the score via the controller,
			// which we will get a callback to the
			// OnRequestSuccessSubmit handler
			retCode = SC_ScoreController_SubmitScore(m_scoreController, scScore);
			if(retCode != SC_OK)
			{
				QTrace("Scoreloop: (ERROR) score controller submit score failed! %s", SC_MapErrorToStr(retCode));
			}
			else
			{
				QTrace("Scoreloop: (DEBUG) submitted result=%d minorResult=%d level=%d mode=%d", m_result, m_minorResult, m_level, g_mode);
			}
		}
		else
		{
			QTrace("Scoreloop: (ERROR) score controller create score failed! %s", SC_MapErrorToStr(retCode));
		}

		// SC_ScoreController_SubmitScore takes the ownership,
		// we can safely release our local score.
		SC_Score_Release(scScore);

		// If our request failed then invoke our
		// failure handler right now.
		if (retCode != SC_OK)
		{
			scoreResult(retCode);
		}
	}
	else
	{
		QTrace("Scoreloop: (WARN) submitted user scores are worse than current scores!");
		scoreResult(SC_INVALID_STATE);
	}
}
#pragma endregion

#pragma region Score Controller (User Submit)
// Static callback for score controller
void QScoreloop::scoreControllerCallbackSubmit(void *userData, SC_Error_t completionStatus)
{
	QTrace("Scoreloop: (DEBUG) scoreControllerCallbackSubmit called");

	QScoreloop* self = static_cast<QScoreloop*> (userData);
	if(self)
	{
		self->scoreResult(completionStatus);
	}
}

// Member handler for scpre controller
void QScoreloop::scoreResult(SC_Error_t completionStatus)
{
	QTrace("Scoreloop: (DEBUG) scoreResult called");

	bool success = completionStatus == SC_OK;
	if(!success)
	{
		QTrace("Scoreloop: (WARN) %s %s", g_resultSubmitEventPhase, SC_MapErrorToStr(completionStatus));
	}

	sendLuaEvent(g_resultSubmitEventPhase, success);
	if(success)
	{
		refreshRank(false);
	}
}
#pragma endregion

#pragma region Rank Controller (User Request)
// Refresh the rank
void QScoreloop::refreshRank(bool initializing)
{
	QTrace("Scoreloop: (DEBUG) refreshRank called");

	// Guard check on initialized
	if(!initializing && !m_initialized)
	{
		QTrace("Scoreloop: (WARN) refreshRank called [%s, %s]",
			m_initialized ? "initialized" : "not initialized",
			initializing ? "initializing" : "not initializing");
		rankingResult(SC_INVALID_STATE);
		return;
	}

	// The return code
	SC_Error_t retCode;

	// Cancel last request if it is still pending.
	retCode = SC_RankingController_Cancel(m_rankingController);
	if(retCode != SC_OK)
	{
		QTrace("Scoreloop: (ERROR) rank controller cancel failed! %s", SC_MapErrorToStr(retCode));
	}

	// Refresh rank from the server
	retCode = SC_RankingController_LoadRankingForUserInMode(
		m_rankingController,
		SC_Session_GetUser(SC_Client_GetSession(m_client)),
		g_mode);
	if(retCode != SC_OK)
	{
		QTrace("Scoreloop: (ERROR) rank controller load failed! %s", SC_MapErrorToStr(retCode));
		rankingResult(retCode);
	}
}

// Static handler for ranking controller
void QScoreloop::rankingControllerCallback(void *userData, SC_Error_t completionStatus)
{
	QTrace("Scoreloop: (DEBUG) rankingControllerCallback called");

	QScoreloop* self = static_cast<QScoreloop*> (userData);
	if(self)
	{
		self->rankingResult(completionStatus);
	}
}

// Member handler for ranking controller
void QScoreloop::rankingResult(SC_Error_t completionStatus)
{
	QTrace("Scoreloop: (DEBUG) rankingResult called");

	if (completionStatus == SC_OK)
	{
		// Get the score object for the user
		SC_Score_h score = SC_RankingController_GetScore(m_rankingController);

		// If we are not available because this is our first
		// pull of the rank users rank from the server then
		// we also have to refresh the users score from that
		// so that we can populate our views.
		if(!m_available)
		{
			// Get the score associated with this rank.
			if (score)
			{
				// Set our local scores based on what has been
				// submitted to the server for the current rank.
				m_result = (unsigned int)SC_Score_GetResult(score);
				m_minorResult = (unsigned int)SC_Score_GetMinorResult(score);
				m_level = SC_Score_GetLevel(score);
			}
			else
			{
				QTrace("Scoreloop: (DEBUG) %s current user has no score!", g_rankRequestEventPhase);
			}

			// Initialize the login name
			SC_Session_h session = SC_Client_GetSession(m_client);
			SC_User_h user = SC_Session_GetUser(session);
			SC_String_h login = SC_User_GetLogin(user);
			if(login)
			{
				m_login = SC_String_GetData(login);
				QTrace("Scoreloop: (DEBUG) login %s", m_login.c_str());
			}

			// Set that we are available since we
			// have now initialized our state.
			m_available = true;
		}

		// Get the rank
		m_rank = SC_RankingController_GetRanking(m_rankingController);
		
		// Send LUA event for rank request
		std::map<std::string, unsigned int> dataMap;
		dataMap.insert(std::pair<std::string, unsigned int>("rank", m_rank));
		dataMap.insert(std::pair<std::string, unsigned int>("level", m_level));
		dataMap.insert(std::pair<std::string, unsigned int>("result", m_result));
		dataMap.insert(std::pair<std::string, unsigned int>("minorResult", m_minorResult));
		sendLuaEvent(g_rankRequestEventPhase, true, dataMap);
	}
	else
	{
		// Send LUA event for rank request
		QTrace("Scoreloop: (WARN) %s %s", g_rankRequestEventPhase, SC_MapErrorToStr(completionStatus));
		sendLuaEvent(g_rankRequestEventPhase, false);
	}
}
#pragma endregion

#pragma region LUA Helpers
// Send a LUA event to the system
void QScoreloop::sendLuaEvent(const char* phase, bool success)
{
	std::map<std::string, unsigned int> dataMap;
	sendLuaEvent(phase, success, dataMap);
}

// Send a LUA event to the system
void QScoreloop::sendLuaEvent(const char* phase, bool success, std::map<std::string, unsigned int>& dataMap)
{
	QTrace("Scoreloop: (DEBUG) sending event phase=%s success=%d", phase, success);
	LUA_EVENT_PREPARE("scoreloop");
    LUA_EVENT_SET_STRING("phase", phase);
	LUA_EVENT_SET_STRING("player", m_login.c_str());
	LUA_EVENT_SET_BOOLEAN("success", success);
	
	// Populate the event with our data
	for (std::map<std::string, unsigned int>::iterator it = dataMap.begin(); it != dataMap.end(); ++it)
	{
		LUA_EVENT_SET_INTEGER(it->first.c_str(), it->second);
	}
	
	LUA_EVENT_SEND();
	lua_pop(g_L, 1); // why do we need this?
}
#pragma endregion

#pragma region Internal Helpers
// Destroy all resources
void QScoreloop::destroy()
{
	QTrace("Scoreloop: (DEBUG) destroy called");

	// ERROR code returned
	SC_Error_t retCode;

	// Release the score controller
	if (m_scoreController)
	{
		QTrace("Scoreloop: (DEBUG) cancelling score controller");
		retCode = SC_ScoreController_Cancel(m_scoreController);
		if(retCode != SC_OK)
		{
			QTrace("Scoreloop: (ERROR) cancel score controller failed! %s", SC_MapErrorToStr(retCode));
		}
		
		QTrace("Scoreloop: (DEBUG) releasing score controller");
		SC_ScoreController_Release(m_scoreController);
		m_scoreController = NULL;
	}

	// Release the ranking controller
	if (m_rankingController)
	{
		QTrace("Scoreloop: (DEBUG) cancelling rank controller");
		retCode = SC_RankingController_Cancel(m_rankingController);
		if(retCode != SC_OK)
		{
			QTrace("Scoreloop: (ERROR) cancel rank controller failed! %s", SC_MapErrorToStr(retCode));
		}

		QTrace("Scoreloop: (DEBUG) releasing rank controller");
		SC_RankingController_Release(m_rankingController);
		m_rankingController = NULL;
	}

	// Release the client
	if(m_client)
	{
		QTrace("Scoreloop: (DEBUG) releasing client");
		SC_Client_Release(m_client);
		m_client = NULL;
	}
}
#pragma endregion