class Pacman extends MovieClip
{
   var active;
   var count;
   var down;
   var game;
   var left;
   var nextx;
   var nexty;
   var right;
   var up;
   var speed = 3;
   var lifeSpan = 200;
   function Pacman()
   {
      super();
      this.stop();
      this.left = false;
      this.right = false;
      this.up = false;
      this.down = false;
      this.count = 0;
   }
   function Activate()
   {
      this.count = 0;
      this.active = true;
      this.play();
      this.left = false;
      this.right = false;
      this.up = false;
      this.down = false;
      this.nextx = this._x;
      this.nexty = this._y;
      this.game.mainTimeline.arrowkeys._visible = true;
   }
   function DeActivate()
   {
      if(this._visible)
      {
         if(!this.game.gotmultishot and !this.game.gotboomerang)
         {
            this.game.OneLessShot();
         }
      }
      this.active = false;
      this._visible = false;
      this._x = this._y = 2000;
      this.game.mainTimeline.arrowkeys._visible = false;
      this.stop();
   }
   function Update()
   {
      if(this.active)
      {
         this._visible = true;
         this.ResolveInput();
         this.Movement();
         this.CheckCollisions();
         this._x = this.nextx;
         this._y = this.nexty;
         this.count = this.count + 1;
         if(this.count > this.lifeSpan)
         {
            this.DeActivate();
         }
      }
   }
   function ResolveInput()
   {
      if(Key.isDown(38))
      {
         this.up = true;
      }
      else
      {
         this.up = false;
      }
      if(Key.isDown(40))
      {
         this.down = true;
      }
      else
      {
         this.down = false;
      }
      if(Key.isDown(37))
      {
         this.left = true;
      }
      else
      {
         this.left = false;
      }
      if(Key.isDown(39))
      {
         this.right = true;
      }
      else
      {
         this.right = false;
      }
   }
   function Movement()
   {
      if(this.right)
      {
         this.nextx = this._x + this.speed;
         this._rotation = 0;
         return undefined;
      }
      if(this.left)
      {
         this.nextx = this._x - this.speed;
         this._rotation = 180;
         return undefined;
      }
      if(this.up)
      {
         this.nexty = this._y - this.speed;
         this._rotation = -90;
         return undefined;
      }
      if(this.down)
      {
         this.nexty = this._y + this.speed;
         this._rotation = 90;
         return undefined;
      }
   }
   function CheckCollisions()
   {
      var _loc4_ = false;
      var _loc3_ = 0;
      var _loc2_;
      while(_loc3_ < this.game.balloons.length)
      {
         _loc2_ = this.game.balloons[_loc3_];
         if(!_loc2_.popped)
         {
            if(GetDistSq(_loc2_._x,_loc2_._y,this._x,this._y) <= 10000)
            {
               if(_loc2_.hitTest(this.nextx,this.nexty,true))
               {
                  if(_loc2_.frozen)
                  {
                     _loc2_.frozen = false;
                  }
                  _loc2_.Pop();
                  if(_loc2_.type != 21)
                  {
                     this.game.PopBalloon(_loc2_);
                  }
                  if(_loc2_.solid)
                  {
                     _loc4_ = true;
                  }
                  trace("eat bloon");
               }
            }
         }
         _loc3_ = _loc3_ + 1;
      }
      if(_loc4_)
      {
         this.nextx = this._x;
         this.nexty = this._y;
      }
   }
}
