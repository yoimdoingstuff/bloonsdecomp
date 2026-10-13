class Tack extends MovieClip
{
   var checkCount;
   var count;
   var dead;
   var game;
   var grav;
   var hitBit;
   var vx;
   var vy;
   function Tack()
   {
      super();
      this.count = 0;
      this.checkCount = 0;
      this.grav = 0.05;
      this.dead = false;
   }
   function Init()
   {
   }
   function Update()
   {
      if(this.dead)
      {
         return undefined;
      }
      if(this.count > 18)
      {
         this._visible = false;
         this.dead = true;
         return undefined;
      }
      this._x += this.vx;
      this._y += this.vy;
      this.vy += this.grav;
      var _loc2_ = Math.atan(this.vy / this.vx);
      _loc2_ = 57.29577951308232 * _loc2_;
      this._rotation = _loc2_ + 90;
      if(this._rotation > 90 and this.vy < 0)
      {
         this._rotation -= 180;
      }
      this.count = this.count + 1;
      this.CheckCollisions();
   }
   function CheckCollisions()
   {
      if(this.checkCount < 4)
      {
         this.checkCount = this.checkCount + 1;
         return undefined;
      }
      this.checkCount = 0;
      var _loc3_ = 0;
      var _loc2_;
      while(_loc3_ < this.game.balloons.length)
      {
         _loc2_ = this.game.balloons[_loc3_];
         if(!_loc2_.popped)
         {
            if(this.GetDistSq(_loc2_._x,_loc2_._y,this._x,this._y) <= 10000)
            {
               if(_loc2_.hitTest(this.hitBit))
               {
                  _loc2_.Pop();
                  if(_loc2_.type == 22)
                  {
                     this._visible = false;
                     this.dead = true;
                     return undefined;
                  }
                  if(_loc2_.type == 21 or _loc2_.frozen or _loc2_.solid)
                  {
                     this.count = 18;
                     return undefined;
                  }
                  this.game.PopBalloon(_loc2_);
               }
            }
         }
         _loc3_ = _loc3_ + 1;
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
