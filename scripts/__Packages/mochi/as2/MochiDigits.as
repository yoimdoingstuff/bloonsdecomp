class mochi.as2.MochiDigits
{
   var Encoder;
   var Fragment;
   var Sibling;
   function MochiDigits(digit, index)
   {
      this.Encoder = 0;
      this.setValue(digit,index);
   }
   function get value()
   {
      return Number(this.toString());
   }
   function set value(v)
   {
      this.setValue(v);
   }
   function addValue(v)
   {
      this.value += v;
   }
   function setValue(digit, index)
   {
      var _loc3_ = digit.toString();
      if(index == undefined || isNaN(index))
      {
         index = 0;
      }
      this.Fragment = _loc3_.charCodeAt(index++) ^ this.Encoder;
      if(index < _loc3_.length)
      {
         this.Sibling = new mochi.as2.MochiDigits(digit,index);
      }
      else
      {
         this.Sibling = null;
      }
      this.reencode();
   }
   function reencode()
   {
      var _loc2_ = int(2147483647 * Math.random());
      this.Fragment ^= _loc2_ ^ this.Encoder;
      this.Encoder = _loc2_;
   }
   function toString()
   {
      var _loc2_ = String.fromCharCode(this.Fragment ^ this.Encoder);
      return this.Sibling == null ? _loc2_ : _loc2_.concat(this.Sibling.toString());
   }
}
