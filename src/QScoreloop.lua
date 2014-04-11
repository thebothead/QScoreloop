-- Create the global scores singleton
scores = {}
scores.initialized = false
scores.submitting = false
scores.refreshing = false
scores.player = ""
scores.result = 0
scores.minorResult = 0
scores.level = 0
scores.rank = 0

-- Scoreloop internal instance
local scoreloop = quickuser.QScoreloop:new()

-- Scoreloop event handler
local function scoreloopHandler(event)
  dbg.print("scoreloop event")
  dbg.printTable(event);
  if event.phase == "userResultSubmit" then
    scores.submitting = false
    if event.success == true then
      scores.refreshing = true
    end
  elseif event.phase == "userRankRequest" then
    scores.refreshing = false
    if event.success == true then
      scores.player = event.player
      scores.result = event.result
      scores.minorResult = event.minorResult
      scores.level = event.level
      scores.rank = event.rank
    end
  end
end
system:addEventListener("scoreloop", scoreloopHandler)

-- Initialize scoreloop
function scores:init(gameIdentifier, gameSecret, gameVersion, currency, languages)
  dbg.print("QScoreloop: initialize")
  dbg.assertFuncVarTypes({
    "string", "string", "string", "string", "string"},
    gameIdentifier, gameSecret, gameVersion, currency, languages)
  if scores.initialized == false then
    scores.refreshing = true
    scoreloop:init(gameIdentifier, gameSecret, gameVersion, currency, languages)
    scores.initialized = true
  end
end

-- Terminate scoreloop
function scores:terminate()
  dbg.print("QScoreloop: terminate")
  if scores.initialized == true then
    scoreloop:terminate()
    scores.submitting = false
    scores.refreshing = false
    scores.initialized = false
  end
end

-- Submit user scores
function scores:submit(result, minorResult, level)
  dbg.print("QScoreloop: submit")
  dbg.assertFuncVarTypes({"number", "number", "number"}, result, minorResult, level)
  if scores.initialized == true and scores.submitting == false then
    scores.submitting = true
    scoreloop:submitResult(result, minorResult, level)
  end
end

-- Refresh the user scores
function scores:refresh()
  dbg.print("QScoreloop: refresh")
  if scores.initialized == true and scores.refreshing == false then
    scores.refreshing = true
    scoreloop:refresh()
  end
end

-- Cancel open requests
function scores:cancel()
  dbg.print("QScoreloop: cancel")
  if scores.initialized == true then
    scoreloop:cancel(false)
    scores.submitting = false
    scores.refreshing = false
  end
end
