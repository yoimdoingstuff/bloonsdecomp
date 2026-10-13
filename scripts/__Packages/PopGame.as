class PopGame
{
   var aimarrow;
   var balCount;
   var balloonHolder;
   var balloons;
   var balloonsLeft;
   var balloonsPopped;
   var bombSound;
   var curLevel;
   var dart;
   var dart2;
   var dart3;
   var easymode;
   var frozenClink;
   var gameState;
   var gotboomerang;
   var gotmultishot;
   var iceSound;
   var ingame;
   var levelEnded;
   var levelObject;
   var levelScore;
   var mainTimeline;
   var metalClink;
   var monkey;
   var myLevel;
   var numBloons;
   var numTacks;
   var pacmanMusic;
   var playarea;
   var playerScore;
   var powercount;
   var powerdirn;
   var screenShake;
   var screenShakeLastFrame;
   var shots;
   var spikeys;
   var tackHolder;
   var tacks;
   var timeLastFrame;
   var timeSinceLastPop;
   var cellWidth = 25;
   var cellHeight = 36;
   var gridWidth = 24;
   var gridHeight = 13;
   var maxpower = 45;
   var minpower = 18;
   function PopGame(timeline)
   {
      this.mainTimeline = timeline;
      this.balloonHolder = this.mainTimeline.balloonHolder;
      this.dart = this.mainTimeline.dart;
      this.dart2 = this.mainTimeline.dart_2;
      this.dart3 = this.mainTimeline.dart_3;
      this.numBloons = 0;
      this.playerScore = 0;
      this.playarea = this.mainTimeline.playarea;
      this.tackHolder = this.mainTimeline.tackHolder;
      this.aimarrow = this.mainTimeline.aimarrow;
      this.levelObject = new Level(this);
      this.dart.game = this;
      this.dart2.game = this;
      this.dart3.game = this;
      this.dart2.extra = true;
      this.dart3.extra = true;
      this.easymode = false;
      this.ingame = false;
      this.SetUpSound();
      this.mainTimeline.onEnterFrame = mx.utils.Delegate.create(this,this.GameLoop);
      this.mainTimeline.bghit.onPress = mx.utils.Delegate.create(this,this.StartAim);
      this.mainTimeline.bghit.onRelease = mx.utils.Delegate.create(this,this.Shoot);
      userProfile = SharedObject.getLocal("bloonsProfile");
      userProfile.data.level1 = "clear";
      userProfile.flush();
   }
   function Init()
   {
      this.levelScore = 0;
      this.levelEnded = false;
      trace("LEVEL = " + this.curLevel);
      trace("thanks = " + this.mainTimeline.thanks_mc);
      if(this.curLevel == 50)
      {
         this.mainTimeline.thanks_mc._visible = true;
         trace(this.mainTimeline.thanks_mc._visible);
      }
      else
      {
         this.mainTimeline.thanks_mc._visible = false;
      }
      this.ClearLevel();
      this.aimarrow._visible = true;
   }
   function ClearAll()
   {
      var _loc2_ = 1;
      while(_loc2_ < 10)
      {
         this.mainTimeline.gameui.ui["dart" + _loc2_]._visible = false;
         _loc2_ = _loc2_ + 1;
      }
      this.powerdirn = 1;
      this.gotboomerang = false;
      this.gotmultishot = false;
      this.mainTimeline.gameui.ui.boompowerup._visible = false;
      this.mainTimeline.gameui.ui.multipowerup._visible = false;
      multipowerup;
      this.aimarrow._x = 1000;
      this.numTacks = 0;
      this.powercount = 0;
      this.numBloons = 0;
      this.balloonsPopped = 0;
      this.timeSinceLastPop = 0;
      this.timeLastFrame = getTimer();
      this.screenShake = this.screenShakeLastFrame = 0;
      this.ClearBalloons();
      this.ClearTacks();
      this.ClearSpikeys();
      removeMovieClip(this.monkey);
      this.balCount = 0;
      this.balloonsLeft = 0;
      this.monkey.throwing = false;
      this.gameState = "normal";
      this.balloons = new Array();
      this.tacks = new Array();
      this.spikeys = new Array();
      this.shots = 0;
      this.dart.Init();
      this.dart2.Init();
      this.dart3.Init();
   }
   function ClearLevel()
   {
      this.ClearAll();
      this.levelObject.Init();
      this.LoadLevel(this.curLevel);
      this.mainTimeline.pacman.DeActivate();
      this.mainTimeline.pacman.game = this;
   }
   function OneMoreShot()
   {
      this.mainTimeline.gameui.ui["dart" + this.shots]._visible = true;
      this.shots = this.shots - 1;
   }
   function OneLessShot()
   {
      if(this.easymode)
      {
         this.CheckEasyModeWin();
         return undefined;
      }
      this.shots = this.shots + 1;
      this.mainTimeline.gameui.ui["dart" + this.shots]._visible = false;
   }
   function CheckEndLevel()
   {
      if(!this.mainTimeline.pacman.active)
      {
         if(this.timeSinceLastPop > 1000)
         {
            if(this.shots >= this.levelObject.darts)
            {
               trace("check win?");
               this.CheckWin();
            }
            if(this.balloonsPopped >= this.numBloons)
            {
               this.CheckWin();
            }
         }
      }
   }
   function StartAim()
   {
      if(this.ingame and !this.mainTimeline.pacman.active)
      {
         if(this.shots < this.levelObject.darts + 1)
         {
            if(!this.dart.launched)
            {
               if(this.gotboomerang)
               {
                  this.dart.gotoAndStop("boomerang");
                  this.gotboomerang = false;
               }
               else
               {
                  this.dart.gotoAndStop("dart");
               }
               this.dart.startLaunch = true;
            }
         }
      }
   }
   function PopBalloon(bloon)
   {
      if(bloon.solid)
      {
         trace("solid");
         return undefined;
      }
      if(bloon.type == 4 or bloon.type == 23)
      {
         return undefined;
      }
      this.balloonsPopped = this.balloonsPopped + 1;
      this.timeSinceLastPop = 0;
      this.mainTimeline.gameui.ui.popped_txt.text = this.balloonsPopped;
      switch(bloon.type)
      {
         case 2:
            this.monkey.eyes.play();
            this.PopTackBalloon(bloon._x,bloon._y);
            break;
         case 3:
            this.monkey.eyes.play();
            this.OneMoreShot();
            break;
         case 5:
            this.monkey.eyes.play();
            this.PopBombBalloon(bloon._x,bloon._y);
            break;
         case 6:
            this.monkey.eyes.play();
            this.PopIceBalloon(bloon._x,bloon._y);
            break;
         case 7:
            this.monkey.eyes.play();
            this.mainTimeline.pacman._x = bloon._x - this.mainTimeline.pacman._width / 4;
            this.mainTimeline.pacman._y = bloon._y - this.mainTimeline.pacman._height / 4;
            this.mainTimeline.pacman.Activate();
            this.pacmanMusic.start();
            break;
         case 8:
            trace("BOOOOMERAnG");
            this.monkey.eyes.play();
            this.gotboomerang = true;
            this.mainTimeline.gameui.ui.boompowerup._visible = true;
            this.OneMoreShot();
            break;
         case 9:
            this.monkey.eyes.play();
            this.gotmultishot = true;
            this.mainTimeline.gameui.ui.multipowerup._visible = true;
            this.OneMoreShot();
            break;
         case 10:
            this.monkey.eyes.play();
            this.PopSpikeyBallBloon(bloon._x,bloon._y);
         default:
            return;
      }
   }
   function Shoot()
   {
      var _loc3_;
      var _loc2_;
      var _loc5_;
      var _loc4_;
      var _loc6_;
      var _loc7_;
      var _loc9_;
      var _loc8_;
      if(this.ingame)
      {
         if(this.shots < this.levelObject.darts + 1)
         {
            if(!this.dart.launched and this.dart.startLaunch)
            {
               this.mainTimeline.gameui.ui.boompowerup._visible = false;
               this.mainTimeline.gameui.ui.multipowerup._visible = false;
               _loc3_ = _xmouse - 10;
               _loc2_ = _ymouse - 10;
               _loc5_ = _loc3_ - this.dart._x;
               _loc4_ = _loc2_ - this.dart._y;
               _loc6_ = Math.sqrt(_loc5_ * _loc5_ + _loc4_ * _loc4_);
               _loc7_ = Math.atan(_loc5_ / _loc4_);
               _loc9_ = Math.cos(_loc7_) * (_loc6_ / 2);
               _loc8_ = Math.sin(_loc7_) * (_loc6_ / 2);
               if(this.gotmultishot)
               {
                  trace("SHOOT EXTRA");
                  this.gotmultishot = false;
                  this.mainTimeline.multipowerup._visible = false;
                  this.dart2.Launch(this.powercount,_loc3_,_loc2_ - 70);
                  this.dart3.Launch(this.powercount,_loc3_,_loc2_ + 70);
               }
               this.monkey.armthrow.gotoAndPlay("throw");
               this.dart.boomerang.gotoAndPlay(2);
               this.aimarrow.inner._yscale = 100;
               this.aimarrow.inner.gotoAndStop(1);
               this.dart.Launch(this.powercount,_loc3_,_loc2_);
               this.powercount = 0;
            }
         }
      }
   }
   function GameLoop()
   {
      this.dart.Update();
      this.dart2.Update();
      this.dart3.Update();
      this.UpdateTacks();
      this.UpdateMonkeyArm();
      this.mainTimeline.pacman.Update();
      this.timeSinceLastPop += getTimer() - this.timeLastFrame;
      this.timeLastFrame = getTimer();
      if(this.ingame)
      {
         this.CheckEndLevel();
      }
      if(this.screenShake != 0)
      {
         this.mainTimeline._y += this.screenShake - this.screenShakeLastFrame;
         this.screenShakeLastFrame = this.screenShake;
         this.screenShake /= -1.1;
         if(Math.abs(this.screenShake) < 1)
         {
            this.screenShake = this.screenShakeLastFrame = 0;
            this.mainTimeline._x = this.mainTimeline._y = 0;
         }
      }
   }
   function UpdateMonkeyArm()
   {
      var _loc6_;
      var _loc4_;
      var _loc5_;
      var _loc3_;
      var _loc2_;
      if(this.monkey and !this.monkey.throwing)
      {
         _loc6_ = Math.sqrt(this.GetDistSq(_xmouse,_ymouse,this.monkey._x,this.monkey._y));
         _loc4_ = this.monkey._y - _ymouse;
         _loc5_ = this.monkey._x - _xmouse;
         _loc3_ = Math.acos(_loc4_ / _loc6_);
         _loc3_ *= 57.29577951308232;
         if(_loc5_ > 0)
         {
            _loc3_ = - _loc3_;
         }
         this.monkey.arm._rotation = _loc3_;
         this.aimarrow._rotation = _loc3_;
         _loc2_ = new Object();
         _loc2_.x = this.monkey.arm.tip._x / 2;
         _loc2_.y = this.monkey.arm.tip._y / 2;
         this.monkey.arm.tip.localToGlobal(_loc2_);
         this.aimarrow._x = _loc2_.x;
         this.aimarrow._y = _loc2_.y;
         if(this.dart.startLaunch)
         {
            this.powercount += 0.5 * this.powerdirn;
            if(this.powercount > this.maxpower)
            {
               this.powercount = this.maxpower;
               this.powerdirn = -1;
            }
            if(this.powercount < this.minpower)
            {
               this.powercount = this.minpower;
               this.powerdirn = 1;
            }
            this.dart._rotation = _loc3_;
            this.dart._x = _loc2_.x;
            this.dart._y = _loc2_.y;
            this.dart._visible = true;
            this.aimarrow.inner.gotoAndStop(2);
            this.aimarrow.inner._yscale = 100 + (this.powercount - this.minpower) * 8;
         }
      }
   }
   function UpdateTacks()
   {
      var _loc2_ = 0;
      while(_loc2_ < this.tacks.length)
      {
         this.tacks[_loc2_].Update();
         _loc2_ = _loc2_ + 1;
      }
      _loc2_ = 0;
      while(_loc2_ < this.spikeys.length)
      {
         this.spikeys[_loc2_].Update();
         _loc2_ = _loc2_ + 1;
      }
   }
   function ClearBalloons()
   {
      var _loc2_;
      while(this.balloons.length > 0)
      {
         _loc2_ = this.balloons.shift();
         _loc2_.removeMovieClip();
      }
   }
   function ClearTacks()
   {
      var _loc2_;
      while(this.tacks.length > 0)
      {
         _loc2_ = this.tacks.shift();
         _loc2_.removeMovieClip();
      }
   }
   function ClearSpikeys()
   {
      var _loc2_;
      while(this.spikeys.length > 0)
      {
         _loc2_ = this.spikeys.shift();
         _loc2_.removeMovieClip();
      }
   }
   function ResetGame()
   {
      this.ClearBalloons();
      this.ClearTacks();
   }
   function CreateBalloons()
   {
      var _loc4_;
      var _loc3_;
      var _loc2_ = 0;
      var _loc1_;
      while(_loc2_ < 15)
      {
         _loc1_ = 0;
         while(_loc1_ < 10)
         {
            _loc4_ = 25 * _loc2_ + 250;
            _loc3_ = 36 * _loc1_ + 70;
            CreateBalloon(_loc4_,_loc3_);
            _loc1_ = _loc1_ + 1;
         }
         _loc2_ = _loc2_ + 1;
      }
   }
   function CreateBloon(col, row, ptype)
   {
      var _loc8_;
      var _loc7_;
      if(ptype > 20)
      {
         _loc8_ = this.GetXFromCol(col);
         _loc7_ = this.GetYFromRow(row);
      }
      else
      {
         _loc8_ = this.GetXFromCol(col + 1);
         _loc7_ = this.GetYFromRow(row + 1);
      }
      var _loc6_ = "";
      var _loc9_;
      if(ptype == 1)
      {
         _loc9_ = random(4) + 1;
         _loc6_ = "-" + _loc9_;
         trace("suffix: " + _loc6_);
      }
      var _loc2_ = this.balloonHolder.attachMovie("bloon" + ptype + _loc6_,"bloon" + this.numTacks++,this.balloonHolder.getNextHighestDepth());
      trace("pa: " + this.playarea._x);
      _loc2_._x = _loc8_ + this.playarea._x;
      _loc2_._y = _loc7_ + this.playarea._y;
      _loc2_.type = ptype;
      if(ptype == 22 or ptype == 23)
      {
         _loc2_.solid = true;
         if(ptype == 23)
         {
            if(this.levelObject.levelData[col + 1][row] == 23 or this.levelObject.levelData[col + 1][row] == 22)
            {
               _loc2_.right.disabled = true;
               _loc2_.right._visible = false;
            }
            if(this.levelObject.levelData[col][row + 1] == 23 or this.levelObject.levelData[col][row + 1] == 22)
            {
               _loc2_.bottom.disabled = true;
               _loc2_.bottom._visible = false;
            }
            if(this.levelObject.levelData[col - 1][row] == 23 or this.levelObject.levelData[col - 1][row] == 22)
            {
               _loc2_.left.disabled = true;
               _loc2_.left._visible = false;
            }
            if(this.levelObject.levelData[col][row - 1] == 23 or this.levelObject.levelData[col][row - 1] == 22)
            {
               _loc2_.top.disabled = true;
               _loc2_.top._visible = false;
            }
            if(_loc2_.right.disabled)
            {
               _loc2_.top.gotoAndStop("two");
               _loc2_.bottom.gotoAndStop("one");
            }
            if(_loc2_.left.disabled)
            {
               _loc2_.top.gotoAndStop("one");
               _loc2_.bottom.gotoAndStop("two");
            }
            if(_loc2_.top.disabled)
            {
               _loc2_.left.gotoAndStop("two");
               _loc2_.right.gotoAndStop("one");
            }
            if(_loc2_.bottom.disabled)
            {
               _loc2_.left.gotoAndStop("one");
               _loc2_.right.gotoAndStop("two");
            }
            if(_loc2_.right.disabled and _loc2_.left.disabled)
            {
               _loc2_.top.gotoAndStop("full");
               _loc2_.bottom.gotoAndStop("full");
            }
            if(_loc2_.top.disabled and _loc2_.bottom.disabled)
            {
               _loc2_.right.gotoAndStop("full");
               _loc2_.left.gotoAndStop("full");
            }
         }
      }
      if(_loc2_.type < 20 and _loc2_.type != 4)
      {
         if(_loc2_.type != 10)
         {
            _loc2_.gotoAndStop(random(80) + 1);
         }
         else
         {
            _loc2_.gotoAndStop(21);
         }
         this.numBloons = this.numBloons + 1;
         this.balloonsLeft = this.balloonsLeft + 1;
      }
      if(ptype == 4)
      {
         this.monkey = _loc2_;
         this.monkey.onPress = function()
         {
         };
         this.levelObject.startx = _loc2_._x;
         this.levelObject.starty = _loc2_._y;
         trace("monkey x = " + this.monkey._x);
         trace("monkey y = " + this.monkey._y);
      }
      else
      {
         _loc2_.cacheAsBitmap = true;
         this.balloons.push(_loc2_);
      }
   }
   function PopTackBalloon(px, py)
   {
      trace("POP TACK BLOON");
      var _loc2_ = 7;
      this.AddTack(0,-1,_loc2_,px,py);
      this.AddTack(0.707,-0.707,_loc2_,px,py);
      this.AddTack(1,0,_loc2_,px,py);
      this.AddTack(0.707,0.707,_loc2_,px,py);
      this.AddTack(0,1,_loc2_,px,py);
      this.AddTack(-0.707,0.707,_loc2_,px,py);
      this.AddTack(-1,0,_loc2_,px,py);
      this.AddTack(-0.707,-0.707,_loc2_,px,py);
   }
   function AddTack(dirx, diry, power, px, py)
   {
      var _loc2_ = this.tackHolder.attachMovie("tack","tack" + this.numTacks++,this.tackHolder.getNextHighestDepth());
      _loc2_._x = px - this.cellWidth / 2;
      _loc2_._y = py - this.cellHeight / 2;
      _loc2_.vx = dirx * power;
      _loc2_.vy = diry * power;
      _loc2_.game = this;
      _loc2_.cacheAsBitmap = true;
      this.tacks.push(_loc2_);
   }
   function PopBombBalloon(px, py)
   {
      var _loc3_ = 10000;
      if(Math.sqrt(this.GetDistSq(this.mainTimeline.pacman._x,this.mainTimeline.pacman._y,px,py)) < _loc3_ / 100)
      {
         this.mainTimeline.pacman.DeActivate();
      }
      this.screenShake = 10;
      var _loc2_ = 0;
      for(; _loc2_ < this.balloons.length; _loc2_ = _loc2_ + 1)
      {
         if(!this.balloons[_loc2_].popped)
         {
            if(this.GetDistSq(this.balloons[_loc2_]._x - this.cellWidth / 2,this.balloons[_loc2_]._y - this.cellHeight / 2,px - this.cellWidth / 2,py - this.cellHeight / 2) < _loc3_)
            {
               if(this.balloons[_loc2_].type == 22 || this.balloons[_loc2_].type == 23)
               {
                  continue;
               }
               this.balloons[_loc2_].solid = false;
               this.balloons[_loc2_].frozen = false;
               this.balloons[_loc2_].Pop();
               if(this.balloons[_loc2_].type == 21)
               {
                  continue;
               }
               if(this.balloons[_loc2_].type != 5 and this.balloons[_loc2_].type != 10)
               {
                  this.balloons[_loc2_].type = 1;
               }
               this.PopBalloon(this.balloons[_loc2_]);
            }
         }
         else
         {
            trace("already popped");
         }
      }
      stopAllSounds();
      this.bombSound.start();
   }
   function PopSpikeyBallBloon(px, py)
   {
      var _loc2_ = this.tackHolder.attachMovie("spikeyball","spikey" + this.numTacks++,this.tackHolder.getNextHighestDepth());
      _loc2_._x = px - this.cellWidth / 2;
      _loc2_._y = py - this.cellHeight / 2;
      _loc2_.game = this;
      _loc2_.cacheAsBitmap = true;
      this.spikeys.push(_loc2_);
   }
   function PopIceBalloon(px, py)
   {
      var _loc3_ = 10000;
      if(this.GetDistSq(this.mainTimeline.pacman._x,this.mainTimeline.pacman._y,px,py) < _loc3_)
      {
         this.mainTimeline.pacman.DeActivate();
      }
      var _loc2_ = 0;
      while(_loc2_ < this.balloons.length)
      {
         if(!this.balloons[_loc2_].popped)
         {
            if(this.GetDistSq(this.balloons[_loc2_]._x + this.cellWidth / 2,this.balloons[_loc2_]._y + this.cellHeight / 2,px + this.cellWidth / 2,py + this.cellHeight / 2) < _loc3_)
            {
               if(this.balloons[_loc2_].type != 6)
               {
                  this.balloons[_loc2_].Freeze();
               }
            }
         }
         else
         {
            trace("already popped");
         }
         _loc2_ = _loc2_ + 1;
      }
      stopAllSounds();
      this.iceSound.start();
   }
   function GetBalloonsLeft()
   {
      return this.balloonsLeft;
   }
   function LoadLevel(name)
   {
      loadLevel = new LoadVars();
      loadLevelResult = new LoadVars();
      loadLevelResult.onLoad = mx.utils.Delegate.create(this,this.LoadResult);
      trace("loading level: " + this.curLevel);
      if(levelsArray[this.curLevel - 1])
      {
         loadLevelResult.decode(levelsArray[this.curLevel - 1]);
         this.LoadResult(true);
      }
      else
      {
         loadLevel.name = name;
         loadLevel.sendAndLoad("http://www.ninjakiwi.com/bloons_leveltest/php/loadlevels.php",loadLevelResult,"POST");
      }
   }
   function LoadResult(success)
   {
      trace("the load was: " + success);
      this.levelObject.name = loadLevelResult.name;
      this.levelObject.target = loadLevelResult.target;
      this.levelObject.darts = loadLevelResult.darts;
      this.levelObject.title = loadLevelResult.title;
      var _loc3_ = 1;
      while(_loc3_ <= this.levelObject.darts)
      {
         this.mainTimeline.gameui.ui["dart" + _loc3_]._visible = true;
         _loc3_ = _loc3_ + 1;
      }
      _loc3_ = 0;
      var _loc2_;
      while(_loc3_ < this.gridWidth)
      {
         _loc2_ = 0;
         while(_loc2_ < this.gridHeight)
         {
            this.levelObject.levelData[_loc3_][_loc2_] = parseInt(loadLevelResult[_loc3_ + "-" + _loc2_]);
            _loc2_ = _loc2_ + 1;
         }
         _loc3_ = _loc3_ + 1;
      }
      _loc3_ = 0;
      while(_loc3_ < this.gridWidth)
      {
         _loc2_ = 0;
         while(_loc2_ < this.gridHeight)
         {
            if(this.levelObject.levelData[_loc3_][_loc2_] != 0)
            {
               this.CreateBloon(_loc3_,_loc2_,this.levelObject.levelData[_loc3_][_loc2_]);
            }
            _loc2_ = _loc2_ + 1;
         }
         _loc3_ = _loc3_ + 1;
      }
      this.mainTimeline.gameui.ui.target_txt.text = this.levelObject.target;
      this.mainTimeline.gameui.ui.leveltitle_txt.text = this.levelObject.name + ". ";
      this.mainTimeline.gameui.ui.leveltitle_txt.text += this.levelObject.title;
      this.mainTimeline.gameui.ui.popped_txt.text = 0;
      var _loc4_ = 0;
      if(userProfile.data["levelbest" + this.curLevel])
      {
         _loc4_ = userProfile.data["levelbest" + this.curLevel];
      }
      else
      {
         userProfile.data["levelbest" + this.curLevel] = 0;
         userProfile.flush();
      }
      this.mainTimeline.gameui.ui.pb_txt.text = this.playerScore;
      this.dart.Init();
      this.dart2.Init();
      this.dart3.Init();
      this.ingame = true;
   }
   function SetLevel(levelname)
   {
      trace("set level to:" + levelname);
      this.curLevel = levelname;
      this.ClearLevel();
   }
   function SetEasyMode(easystate)
   {
      this.easymode = easystate;
   }
   function CheckWin()
   {
      var _loc2_;
      if(this.levelObject.target - this.balloonsPopped > 0)
      {
         this.ingame = false;
         this.mainTimeline.losepanel_mc.gotoAndPlay("drawOn");
      }
      else if(this.balloonsPopped >= this.levelObject.target)
      {
         this.ingame = false;
         userProfile.data["level" + (this.curLevel + 1)] = "clear";
         userProfile.flush();
         _loc2_ = Math.round(this.balloonsPopped / this.numBloons * 1000) / 10;
         if(_loc2_ > Number(userProfile.data["levelbest" + this.curLevel]))
         {
            trace("new PB!");
            userProfile.data["levelbest" + this.curLevel] = _loc2_;
            userPforile.flush();
         }
         this.levelScore = _loc2_;
         if(!this.levelEnded)
         {
            this.levelEnded = true;
            this.playerScore += _loc2_;
         }
         if(this.curLevel != 50)
         {
            this.mainTimeline.winpanel_mc.gotoAndPlay("drawOn");
         }
         else
         {
            this.monkey._visible = false;
            this.aimarrow._visible = false;
         }
      }
   }
   function CheckEasyModeWin()
   {
      if(this.balloonsPopped >= this.levelObject.target)
      {
         this.ingame = false;
         this.playerScore = 0;
         if(this.curLevel != 50)
         {
            this.mainTimeline.winpanel_mc.gotoAndPlay("drawOn");
         }
         else
         {
            this.monkey._visible = false;
            this.aimarrow._visible = false;
         }
      }
   }
   function GetColFromX(px)
   {
      px -= this.playarea._x;
      px /= this.cellWidth;
      px = Math.floor(px);
      return px;
   }
   function GetRowFromY(py)
   {
      py -= this.playarea._y;
      py /= this.cellHeight;
      py = Math.floor(py);
      return py;
   }
   function GetXFromCol(col)
   {
      return col * this.cellWidth;
   }
   function GetYFromRow(row)
   {
      return row * this.cellHeight;
   }
   function GetDistSq(x1, y1, x2, y2)
   {
      var _loc2_ = x1 - x2;
      var _loc1_ = y1 - y2;
      var _loc3_ = _loc2_ * _loc2_ + _loc1_ * _loc1_;
      return _loc3_;
   }
   function SetUpSound()
   {
      this.bombSound = new Sound(this.mainTimeline);
      this.bombSound.attachSound("bomb");
      this.iceSound = new Sound(this.mainTimeline);
      this.iceSound.attachSound("ice");
      this.frozenClink = new Sound(this.mainTimeline);
      this.frozenClink.attachSound("clink");
      this.metalClink = new Sound(this.mainTimeline);
      this.metalClink.attachSound("metalClink");
      this.pacmanMusic = new Sound(this.mainTimeline);
      this.pacmanMusic.attachSound("pacsong");
   }
   function UpdateThumbs()
   {
      var _loc4_ = this.mainTimeline.thumbs;
      var _loc2_ = 1;
      var _loc3_;
      while(_loc2_ < 51)
      {
         if(userProfile.data["level" + _loc2_] == "clear")
         {
            _loc3_ = _loc4_[_loc2_ + ""];
            _loc3_.gotoAndStop(2);
            var game = this;
            _loc3_.myLevel = _loc2_;
            _loc3_.onRelease = function()
            {
               game.curLevel = this.myLevel;
               game.mainTimeline.play();
            };
         }
         _loc2_ = _loc2_ + 1;
      }
   }
}
