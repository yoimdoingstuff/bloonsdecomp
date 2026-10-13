on(press){
   _parent._parent.popgame.playerScore -= _parent._parent.popgame.levelScore;
   _parent._parent.popgame.levelScore = 0;
   _parent.dest = "play";
   _parent.gotoAndPlay("drawOff");
}
