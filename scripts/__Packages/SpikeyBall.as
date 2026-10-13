class SpikeyBall extends MovieClip
{
   var game;
   var grav;
   var hitbit;
   var rebounds;
   var rv;
   var stuck;
   var vx;
   var vy;
   function SpikeyBall()
   {
      super();
      this.grav = 0.8;
      this.rv = random(10) - 5;
      this.vx = this.vy = 0;
      this.rebounds = 0;
      this.stuck = false;
   }
   function Init()
   {
   }
   function Update()
   {
      if(this.stuck)
      {
         this.vx = this.vy = this.rv = 0;
         this._alpha -= 2;
         if(this._alpha <= 0)
         {
            this.removeMovieClip();
         }
         return undefined;
      }
      this._x += this.vx;
      this._y += this.vy;
      this._rotation += this.rv;
      this.vy += this.grav;
      this.CheckCollisions();
   }
   function CheckCollisions()
   {
      var _loc4_ = 0;
      var _loc2_;
      var _loc3_;
      var _loc6_;
      var _loc5_;
      while(_loc4_ < this.game.balloons.length)
      {
         _loc2_ = this.game.balloons[_loc4_];
         if(!_loc2_.popped)
         {
            if(this.GetDistSq(_loc2_._x,_loc2_._y,this._x,this._y) <= 10000)
            {
               if(_loc2_.hitTest(this.hitbit))
               {
                  _loc2_.Pop();
                  if(_loc2_.type == 21)
                  {
                     this.rebounds = this.rebounds + 1;
                     if(this.rebounds > 5)
                     {
                        this.stuck = true;
                     }
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
                     this.rebounds = this.rebounds + 1;
                     if(this.rebounds > 6)
                     {
                        this.stuck = true;
                     }
                     if(_loc2_.bottom.hitTest(this))
                     {
                        this.vy = -0.65 * this.vy;
                     }
                     else if(_loc2_.top.hitTest(this))
                     {
                        this.vy = -0.65 * this.vy;
                     }
                     else if(_loc2_.left.hitTest(this))
                     {
                        this.vx = -0.65 * this.vx;
                     }
                     else if(_loc2_.right.hitTest(this))
                     {
                        this.vx = -0.65 * this.vx;
                     }
                     return undefined;
                  }
                  if(_loc2_.type == 23 and !this.stuck)
                  {
                     _loc3_ = 0.95;
                     this.vy *= 0.95;
                     this.vx *= 0.95;
                     this.rebounds += 0.5;
                     if(this.rebounds > 5)
                     {
                        this.stuck = true;
                     }
                     _loc6_ = _loc2_._x + _loc2_._width / 2;
                     _loc5_ = _loc2_._y + _loc2_._height / 2;
                     if(!_loc2_.left.disabled)
                     {
                        if(_loc2_.left.hitTest(this))
                        {
                           _loc2_.gotoAndPlay("lefthit");
                           this.vx = (- _loc3_) * this.vx;
                        }
                     }
                     if(!_loc2_.right.disabled)
                     {
                        if(_loc2_.right.hitTest(this))
                        {
                           _loc2_.gotoAndPlay("righthit");
                           this.vx = (- _loc3_) * this.vx;
                        }
                     }
                     if(!_loc2_.bottom.disabled)
                     {
                        if(_loc2_.bottom.hitTest(this))
                        {
                           _loc2_.gotoAndPlay("bothit");
                           this.vy = (- _loc3_) * this.vy;
                        }
                     }
                     if(!_loc2_.top.disabled)
                     {
                        if(_loc2_.top.hitTest(this))
                        {
                           _loc2_.gotoAndPlay("tophit");
                           this.vy = (- _loc3_) * this.vy;
                        }
                     }
                     return undefined;
                  }
                  this.game.PopBalloon(_loc2_);
                  if(_loc2_.type == 5)
                  {
                     this.removeMovieClip();
                  }
               }
            }
         }
         _loc4_ = _loc4_ + 1;
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
