newgame.onRollOver = function()
{
   this.bloon.gotoAndPlay("drawOn");
};
newgame.onRollOut = function()
{
   this.bloon.gotoAndPlay("drawOff");
};
showhs.onRollOver = function()
{
   this.bloon.gotoAndPlay("drawOn");
};
showhs.onRollOut = function()
{
   this.bloon.gotoAndPlay("drawOff");
};
moregames.onRollOver = function()
{
   this.bloon.gotoAndPlay("drawOn");
};
moregames.onRollOut = function()
{
   this.bloon.gotoAndPlay("drawOff");
};
howtoplay.onRollOver = function()
{
   this.bloon.gotoAndPlay("drawOn");
};
howtoplay.onRollOut = function()
{
   this.bloon.gotoAndPlay("drawOff");
};
morelevels.onRollOver = function()
{
   this.bloon.gotoAndPlay("drawOn");
};
morelevels.onRollOut = function()
{
   this.bloon.gotoAndPlay("drawOff");
};
bloonsworld.onRollOver = function()
{
   this.bloon.gotoAndPlay("drawOn");
};
bloonsworld.onRollOut = function()
{
   this.bloon.gotoAndPlay("drawOff");
};
newgame.onRelease = function()
{
   _parent.sounds.randompop.play();
   trace("_parent = " + _parent.sounds.randompop);
   _parent.gotoAndPlay("levelselect");
};
showhs.onRelease = function()
{
   _parent.sounds.randompop.play();
   mochi.as2.MochiScores.showLeaderboard({boardID:"f2caf8bcdb75ce5a",score:_parent.popgame.playerScore});
   _parent.popgame.playerScore = 0;
};
howtoplay.onRelease = function()
{
   _parent.sounds.randompop.play();
   getURL("http://ninjakiwi.com/Games/Bloons-Games/Bloons2.html", "_blank");
};
moregames.onRelease = function()
{
   _parent.sounds.randompop.play();
   getURL("http://www.ninjakiwi.com", "_blank");
};
bloonsworld.onRelease = function()
{
   _parent.sounds.randompop.play();
   getURL("http://www.myninjakiwi.com", "_blank");
};
