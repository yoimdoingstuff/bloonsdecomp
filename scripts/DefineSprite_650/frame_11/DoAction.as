stop();
inner.gotoAndPlay(2);
trace("[ " + _parent.popgame.numBloons + " ]");
trace(_parent.popgame.balloonsPopped);
theresult = Math.round(_parent.popgame.balloonsPopped / _parent.popgame.numBloons * 1000) / 10;
inner.score_txt.text = theresult + "%";
dest = "main";
