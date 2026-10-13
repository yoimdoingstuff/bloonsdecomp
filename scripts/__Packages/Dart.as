class Dart extends MovieClip
{
   var boomcount;
   var boomerang;
   var extra;
   var game;
   var grav;
   var hitBit;
   var lastx;
   var lasty;
   var launched;
   var launchpower;
   var maxspeed;
   var nextx;
   var nexty;
   var rebounds;
   var startLaunch;
   var stuck;
   var vx;
   var vy;
   function Dart()
   {
      super();
      this.grav = 0.7;
      this.extra = false;
   }
   function Init()
   {
      this.launched = false;
      this.startLaunch = false;
      this._visible = false;
      checkthis = false;
      this.launchpower = 0;
      this.vx = this.vy = 0;
      this.rebounds = 0;
      this.stuck = false;
      this.maxspeed = 20;
      this.boomcount = 0;
      this._alpha = 100;
      this._x = this.lastx = this.game.levelObject.startx + 15;
      this._y = this.lasty = this.game.levelObject.starty + 15;
      this.boomerang.gotoAndStop(1);
   }
   function Launch(power, endx, endy)
   {
      if(this._currentframe == 2)
      {
         power /= 2;
         if(power < 9)
         {
            power = 9;
         }
      }
      else
      {
         power /= 1.5;
         if(power < 13)
         {
            power = 13;
         }
      }
      this.launchpower = power;
      this._visible = true;
      this.startLaunch = false;
      var _loc3_ = Math.sqrt(this.GetDistSq(this._x,this._y,endx,endy));
      var _loc5_ = endx - this._x;
      var _loc4_ = this._y - endy;
      _loc5_ /= _loc3_;
      _loc4_ /= _loc3_;
      this._rotation = this.game.monkey.arm._rotation;
      this.launched = true;
      this.vx = _loc5_ * power;
      this.vy = (- _loc4_) * power;
   }
   function Update()
   {
      var _loc3_;
      var _loc2_;
      var _loc5_;
      var _loc4_;
      var _loc6_;
      if(this.launched)
      {
         if(this._currentframe == 1)
         {
            if(this._x > Stage.width - 20 or this._y > Stage.height + 50 or this._x < -50)
            {
               if(!this.extra)
               {
                  this.game.OneLessShot();
               }
               this.Init();
               return undefined;
            }
         }
         if(this.stuck)
         {
            this.boomerang.stop();
            this._alpha -= 2;
            if(this._alpha <= 0)
            {
               if(!this.extra)
               {
                  this.game.OneLessShot();
               }
               this.Init();
            }
            return undefined;
         }
         this.lastx = this._x;
         this.lasty = this._y;
         if(this._currentframe == 2)
         {
            _loc3_ = this.game.monkey._x + 20 - this._x;
            _loc2_ = this.game.monkey._y + 20 - this._y;
            this.boomcount = this.boomcount + 1;
            this.vx += _loc3_ * 0.001;
            this.vy += _loc2_ * 0.001;
            if(this.boomcount < 30)
            {
               this.vy += this.grav / 2;
            }
            else if(!(this.boomcount < 60 or this._y < this.game.monkey._y))
            {
               _loc5_ = Math.sqrt(_loc3_ * _loc3_ + _loc2_ * _loc2_);
               _loc3_ /= _loc5_;
               _loc2_ /= _loc5_;
               this.vx = _loc3_ * this.launchpower / 2;
               this.vy = _loc2_ * this.launchpower / 2;
            }
            _loc4_ = Math.sqrt(this.vx * this.vx + this.vy * this.vy);
            if(_loc4_ > this.maxspeed)
            {
               _loc6_ = this.maxspeed / _loc4_;
               this.vx *= _loc6_;
               this.vy *= _loc6_;
            }
         }
         else
         {
            this.vy += this.grav;
         }
         this.nextx = this._x + this.vx;
         this.nexty = this._y + this.vy;
         this.CheckCollisions();
         this.UpdateAngle();
         this._x += this.vx;
         this._y += this.vy;
      }
      else if(this.startLaunch)
      {
         this._visible = true;
      }
   }
   function UpdateAngle()
   {
      if(this._currentframe == 2)
      {
         return undefined;
      }
      var _loc2_ = Math.atan(this.vy / this.vx);
      _loc2_ = 57.29577951308232 * _loc2_;
      this._rotation = _loc2_ + 90;
      if(this._rotation > 90 and this.vy < 0)
      {
         this._rotation -= 180;
      }
      if(this.vx < 0 and this.vy > 0)
      {
         this._rotation -= 180;
      }
   }
   function CheckCollisions()
   {
      var _loc9_ = 0;
      var _loc2_;
      var _loc5_;
      var _loc3_;
      var _loc4_;
      var _loc8_;
      var _loc6_;
      var _loc7_;
      while(_loc9_ < this.game.balloons.length)
      {
         _loc2_ = this.game.balloons[_loc9_];
         if(!_loc2_.popped)
         {
            if(this.GetDistSq(_loc2_._x,_loc2_._y,this._x,this._y) <= 10000)
            {
               _loc5_ = false;
               if(this._currentframe == 2)
               {
                  if(_loc2_.hitTest(this.hitBit))
                  {
                     _loc2_.Pop();
                     _loc5_ = true;
                     if(_loc2_.type == 21)
                     {
                        this.stuck = true;
                        return undefined;
                     }
                     if(_loc2_.type == 22 and !this.stuck)
                     {
                        this.game.metalClink.start();
                        this.stuck = true;
                        return undefined;
                     }
                     if(_loc2_.frozen and !this.stuck)
                     {
                        this.game.frozenClink.start();
                        dart.stuck = true;
                     }
                     if(_loc2_.type == 23 and !this.stuck)
                     {
                        dart.stuck = true;
                        if(!_loc2_.left.disabled)
                        {
                           if(_loc2_.left.hitTest(this.hitBit))
                           {
                              _loc2_.gotoAndPlay("lefthit");
                           }
                        }
                        if(!_loc2_.right.disabled)
                        {
                           if(_loc2_.right.hitTest(this.hitBit))
                           {
                              _loc2_.gotoAndPlay("righthit");
                           }
                        }
                        if(!_loc2_.bottom.disabled)
                        {
                           if(_loc2_.bottom.hitTest(this.hitBit))
                           {
                              _loc2_.gotoAndPlay("bothit");
                           }
                        }
                        if(!_loc2_.top.disabled)
                        {
                           if(_loc2_.top.hitTest(this.hitBit))
                           {
                              _loc2_.gotoAndPlay("tophit");
                           }
                        }
                     }
                  }
               }
               else
               {
                  _loc3_ = new Object();
                  _loc3_.x = this.hitBit._x;
                  _loc3_.y = this.hitBit._y;
                  this.hitBit.localToGlobal(_loc3_);
                  _loc4_ = 1;
                  while(_loc4_ < 4)
                  {
                     _loc8_ = _loc4_ * (this.vx / 3);
                     if(_loc2_.hitTest(_loc3_.x + _loc8_,_loc3_.y,false))
                     {
                        _loc2_.Pop();
                        _loc5_ = true;
                        if(_loc2_.type == 21)
                        {
                           this.stuck = true;
                           return undefined;
                        }
                        if(_loc2_.type == 22 and !this.stuck)
                        {
                           this.game.metalClink.start();
                           this.stuck = true;
                           return undefined;
                        }
                        if(_loc2_.frozen and !this.stuck)
                        {
                           this.game.frozenClink.start();
                           this.vy *= 0.9;
                           this.vx *= 0.9;
                           this.vx *= 0.65;
                           this.vx *= -1;
                           this.UpdateAngle();
                           this.rebounds = this.rebounds + 1;
                           if(this.rebounds > 12)
                           {
                              this.stuck = true;
                           }
                        }
                        if(_loc2_.type == 23 and !this.stuck)
                        {
                           _loc6_ = 0.85;
                           trace("HIT RUBBER X");
                           this.vy *= 0.95;
                           this.vx *= 0.95;
                           this.vx *= _loc6_;
                           this.vx *= -1;
                           this.rebounds += 0.5;
                           if(this.rebounds > 12)
                           {
                              this.stuck = true;
                           }
                           if(!_loc2_.left.disabled)
                           {
                              if(_loc2_.left.hitTest(this.hitBit))
                              {
                                 _loc2_.gotoAndPlay("lefthit");
                              }
                           }
                           if(!_loc2_.right.disabled)
                           {
                              if(_loc2_.right.hitTest(this.hitBit))
                              {
                                 _loc2_.gotoAndPlay("righthit");
                              }
                           }
                           if(!_loc2_.bottom.disabled)
                           {
                              if(_loc2_.bottom.hitTest(this.hitBit))
                              {
                                 _loc2_.gotoAndPlay("bothit");
                              }
                           }
                           if(!_loc2_.top.disabled)
                           {
                              if(_loc2_.top.hitTest(this.hitBit))
                              {
                                 _loc2_.gotoAndPlay("tophit");
                              }
                           }
                           this.UpdateAngle();
                        }
                        break;
                     }
                     _loc4_ = _loc4_ + 1;
                  }
                  _loc4_ = 1;
                  while(_loc4_ < 4)
                  {
                     _loc7_ = _loc4_ * (this.vy / 3);
                     if(_loc2_.hitTest(_loc3_.x,_loc3_.y + _loc7_,false))
                     {
                        _loc2_.Pop();
                        _loc5_ = true;
                        if(_loc2_.type == 21)
                        {
                           this.stuck = true;
                           return undefined;
                        }
                        if(_loc2_.type == 22 and !this.stuck)
                        {
                           this.game.metalClink.start();
                           this.stuck = true;
                           return undefined;
                        }
                        if(_loc2_.frozen and !this.stuck)
                        {
                           this.game.frozenClink.start();
                           this.vy *= 0.9;
                           this.vx *= 0.9;
                           this.vy *= 0.65;
                           this.vy *= -1;
                           this.UpdateAngle();
                           this.rebounds = this.rebounds + 1;
                           if(this.rebounds > 12)
                           {
                              this.stuck = true;
                           }
                        }
                        if(_loc2_.type == 23 and !this.stuck)
                        {
                           _loc6_ = 0.85;
                           trace("HIT RUBBER Y");
                           this.vy *= 0.95;
                           this.vx *= 0.95;
                           this.vy *= _loc6_;
                           this.vy *= -1;
                           this.rebounds += 0.5;
                           if(this.rebounds > 12)
                           {
                              this.stuck = true;
                           }
                           if(!_loc2_.left.disabled)
                           {
                              if(_loc2_.left.hitTest(this.hitBit))
                              {
                                 _loc2_.gotoAndPlay("lefthit");
                              }
                           }
                           if(!_loc2_.right.disabled)
                           {
                              if(_loc2_.right.hitTest(this.hitBit))
                              {
                                 _loc2_.gotoAndPlay("righthit");
                              }
                           }
                           if(!_loc2_.bottom.disabled)
                           {
                              if(_loc2_.bottom.hitTest(this.hitBit))
                              {
                                 _loc2_.gotoAndPlay("bothit");
                              }
                           }
                           if(!_loc2_.top.disabled)
                           {
                              if(_loc2_.top.hitTest(this.hitBit))
                              {
                                 _loc2_.gotoAndPlay("tophit");
                              }
                           }
                           this.UpdateAngle();
                        }
                        break;
                     }
                     _loc4_ = _loc4_ + 1;
                  }
               }
               if(_loc5_)
               {
                  if(_loc2_.frozen or _loc2_.type == 23)
                  {
                     return undefined;
                  }
                  this.game.PopBalloon(_loc2_);
                  if(_loc2_.type == 5)
                  {
                     if(this == this.game.dart)
                     {
                        this.game.OneLessShot();
                     }
                     this.Init();
                  }
               }
            }
         }
         _loc9_ = _loc9_ + 1;
      }
      if(this.boomcount > 20 and this.game.monkey.arm.hitTest(this.hitBit))
      {
         this.boomerang.gotoAndStop(1);
         this.stuck = true;
      }
   }
   function GetDistSq(x1, y1, x2, y2)
   {
      var _loc2_ = x1 - x2;
      var _loc1_ = y1 - y2;
      var _loc3_ = _loc2_ * _loc2_ + _loc1_ * _loc1_;
      return _loc3_;
   }
}
