class Level
{
   var darts;
   var game;
   var levelData;
   var name;
   var num;
   var startx;
   var starty;
   var target;
   function Level(gameref)
   {
      this.game = gameref;
      this.startx = 50;
      this.starty = Stage.height - 50;
   }
   function Init()
   {
      trace("init level object");
      this.levelData = new Array();
      this.name = "";
      this.num = 0;
      this.target = 0;
      this.darts = 0;
      this.FillUpArray();
   }
   function FillUpArray()
   {
      this.levelData = new Array(this.game.gridWidth);
      i = 0;
      while(i < this.game.gridWidth)
      {
         this.levelData[i] = new Array(this.game.gridHeight);
         j = 0;
         while(j < this.game.gridHeight)
         {
            this.levelData[i][j] = 0;
            j++;
         }
         i++;
      }
   }
}
