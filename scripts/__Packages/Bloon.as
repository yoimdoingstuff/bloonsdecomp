class Bloon extends MovieClip
{
   var frozen;
   var inner;
   var popped;
   var solid;
   function Bloon()
   {
      super();
      this.frozen = false;
      this.popped = false;
      this.solid = false;
   }
   function Pop()
   {
      if(this.frozen or this.solid or this.popped)
      {
         return undefined;
      }
      this.popped = true;
      this.gotoAndPlay("pop");
   }
   function Freeze()
   {
      if(!this.frozen)
      {
         this.frozen = true;
         this.inner.freeze.gotoAndPlay("drawOn");
      }
   }
}
