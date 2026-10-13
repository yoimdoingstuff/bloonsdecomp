class mochi.as2.MochiScores
{
   static var boardID;
   static var onClose;
   static var onError;
   function MochiScores()
   {
   }
   static function setBoardID(boardID)
   {
      mochi.as2.MochiServices.warnID(boardID,true);
      mochi.as2.MochiScores.boardID = boardID;
      mochi.as2.MochiServices.send("scores_setBoardID",{boardID:boardID});
   }
   static function showLeaderboard(options)
   {
      options.clip = mochi.as2.MochiServices.clip;
      if(options.clip != mochi.as2.MochiServices.clip || mochi.as2.MochiServices.childClip._target == undefined)
      {
         mochi.as2.MochiServices.disconnect();
         trace("WARNING!  This application is attempting to connect to MochiServices inside a showLeaderboard call!");
         trace("make sure MochiServices.connect is called as early in the application runtime as possible.");
         mochi.as2.MochiServices.connect(mochi.as2.MochiServices.id,options.clip);
      }
      delete options.clip;
      if(options.name != null)
      {
         if(typeof options.name == "object")
         {
            if(options.name.text != undefined)
            {
               options.name = options.name.text;
            }
         }
      }
      var _loc1_;
      if(options.score != null)
      {
         if(options.score instanceof TextField)
         {
            if(options.score.text != undefined)
            {
               options.score = options.score.text;
            }
         }
         else if(options.score instanceof mochi.as2.MochiDigits)
         {
            options.score = options.score.value;
         }
         _loc1_ = Number(options.score);
         if(isNaN(_loc1_))
         {
            trace("ERROR: Submitted score \'" + options.score + "\' will be rejected, score is \'Not a Number\'");
         }
         else if(_loc1_ == -Infinity || _loc1_ == Infinity)
         {
            trace("ERROR: Submitted score \'" + options.score + "\' will be rejected, score is an infinite");
         }
         else
         {
            if(Math.floor(_loc1_) != _loc1_)
            {
               trace("WARNING: Submitted score \'" + options.score + "\' will be truncated");
            }
            options.score = _loc1_;
         }
      }
      if(options.onDisplay != null)
      {
         options.onDisplay();
      }
      else
      {
         mochi.as2.MochiServices.clip.stop();
      }
      if(options.onClose != null)
      {
         mochi.as2.MochiScores.onClose = options.onClose;
      }
      else
      {
         mochi.as2.MochiScores.onClose = function()
         {
         };
      }
      if(options.onError != null)
      {
         mochi.as2.MochiScores.onError = options.onError;
      }
      else
      {
         mochi.as2.MochiScores.onError = mochi.as2.MochiScores.onClose;
      }
      if(options.boardID == null)
      {
         if(mochi.as2.MochiScores.boardID != null)
         {
            options.boardID = mochi.as2.MochiScores.boardID;
         }
      }
      mochi.as2.MochiServices.warnID(options.boardID,true);
      trace("[MochiScores] NOTE: Security Sandbox Violation errors below are normal");
      mochi.as2.MochiServices.send("scores_showLeaderboard",{options:options},null,mochi.as2.MochiScores.doClose);
   }
   static function closeLeaderboard()
   {
      mochi.as2.MochiServices.send("scores_closeLeaderboard");
   }
   static function getPlayerInfo(callbackObj, callbackMethod)
   {
      mochi.as2.MochiServices.send("scores_getPlayerInfo",null,callbackObj,callbackMethod);
   }
   static function submit(score, name, callbackObj, callbackMethod)
   {
      score = Number(score);
      if(isNaN(score))
      {
         trace("ERROR: Submitted score \'" + String(score) + "\' will be rejected, score is \'Not a Number\'");
      }
      else if(score == -Infinity || score == Infinity)
      {
         trace("ERROR: Submitted score \'" + String(score) + "\' will be rejected, score is an infinite");
      }
      else
      {
         if(Math.floor(score) != score)
         {
            trace("WARNING: Submitted score \'" + String(score) + "\' will be truncated");
         }
         score = Number(score);
      }
      mochi.as2.MochiServices.send("scores_submit",{score:score,name:name},callbackObj,callbackMethod);
   }
   static function requestList(callbackObj, callbackMethod)
   {
      mochi.as2.MochiServices.send("scores_requestList",null,callbackObj,callbackMethod);
   }
   static function scoresArrayToObjects(scores)
   {
      var _loc5_ = {};
      var _loc1_;
      var _loc4_;
      var _loc2_;
      var _loc6_;
      for(var _loc8_ in scores)
      {
         if(typeof scores[_loc8_] == "object")
         {
            if(scores[_loc8_].cols != null && scores[_loc8_].rows != null)
            {
               _loc5_[_loc8_] = [];
               _loc2_ = scores[_loc8_];
               _loc4_ = 0;
               while(_loc4_ < _loc2_.rows.length)
               {
                  _loc6_ = {};
                  _loc1_ = 0;
                  while(_loc1_ < _loc2_.cols.length)
                  {
                     _loc6_[_loc2_.cols[_loc1_]] = _loc2_.rows[_loc4_][_loc1_];
                     _loc1_ = _loc1_ + 1;
                  }
                  _loc5_[_loc8_].push(_loc6_);
                  _loc4_ = _loc4_ + 1;
               }
            }
            else
            {
               _loc5_[_loc8_] = {};
               for(var _loc7_ in scores[_loc8_])
               {
                  _loc5_[_loc8_][_loc7_] = scores[_loc8_][_loc7_];
               }
            }
         }
         else
         {
            _loc5_[_loc8_] = scores[_loc8_];
         }
      }
      return _loc5_;
   }
   static function doClose(args)
   {
      if(args.error == true)
      {
         if(args.errorCode == undefined)
         {
            args.errorCode = "IOError";
         }
         mochi.as2.MochiScores.onError.apply(null,[args.errorCode]);
      }
      else
      {
         mochi.as2.MochiScores.onClose.apply();
      }
   }
}
