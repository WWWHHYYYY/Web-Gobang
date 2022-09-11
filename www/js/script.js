var game = {
    centent: document.getElementById("centent"),
    canvas: document.getElementById("canvas"),
    ctx: canvas.getContext("2d"),
    regret_chess: document.getElementById("button"),
    start_match: document.getElementById("match"),
    anew: document.getElementById("anew"),
    state: document.getElementById("state"),
    sChesee: document.getElementsByClassName("state-chess")[0],
    cName: document.getElementsByClassName("chessName")[0],
    winner: document.getElementById("winner"),
    winChesee: this.winner.getElementsByClassName("state-chess")[0],
    winName: this.winner.getElementsByClassName("chessName")[0],
    e: 0,
    chess_Board: [],
    chess_Name: ["黑棋", "白棋"],
    h: [],
    um: 0,
    lianz: [],
    winXY: [[1, 0], [0, 1], [1, 1], [1, -1]],
    chessOff: true,
    user_id: -1,
    room_id: -1,
    my_chess_name: "",
    match_status : -1,
    peer_step : "",
    match_flag : true,

    drawLine: function () {
        //动态的向后端请求user_id
        $.ajax({
            url: "/GetUserId",
            type: "Get",
            dataType: "JSON",
            //async : false ==> 禁止异步请求   chrome 有可能会禁止 ajax 的同步请求
            async: false,
            success: function (data) {
                if (data.user_id > 0) {
                    game.user_id = data.user_id;
                } else {
                    alert("请先进行登录");
                    window.location.href = "index.html";
                }
            },
        });

        for (var i = 1; i <= 14; i++) {
            game.ctx.moveTo(i * 30 + .5, 420)
            game.ctx.lineTo(i * 30 + .5, 30)
            game.ctx.moveTo(30, i * 30 + .5)
            game.ctx.lineTo(420, i * 30 + .5)
            game.ctx.strokeStyle = "#C0A27B";
            game.ctx.stroke()
        }
        for (var i = 0; i <= 13; i++) {
            game.chess_Board[i] = [];
            game.lianz[i] = [];
            for (var j = 0; j <= 13; j++) {
                game.chess_Board[i][j] = 0;
                game.lianz[i][j] = 0;
            }
        }
    },

    SetMatch: function () {
        if(game.match_flag === false)
        {
          return;
        }
        console.log("SetMatch Func , 开始匹配...");
        $.ajax({
            url: "/SetMatch",
            type: "Get",
            dataType: "JSON",
            //async : false ==> 禁止异步请求   chrome 有可能会禁止 ajax 的同步请求
            async: false,
            success: function (data) {
                if (data.status === 0) {
                    console.log("SetMatch Func , 开始匹配状态设置成功...");
                    game.GetMatchResult()
                } else {
                    alert("匹配失败， 重新匹配...");
                }
            },
        });
    },

    GetMatchResult: function (){
        //循环的获取匹配结果
        //向后台循环发送获取匹配结果， 直到匹配成功
        let s = 0;
        let _time = setInterval(() => {
            console.log("GetMatchResult, " + s++);
            $.ajax({
                url: "/Match",
                type: "Get",
                dataType: "JSON",
                //async : false ==> 禁止异步请求   chrome 有可能会禁止 ajax 的同步请求
                async: false,
                success: function (data) {
                    game.match_status = data.status;
                    console.log(data.status);
                    if (data.status === 1) {
                        /*
                        *   data = {
                        *      status : 匹配状态
                        *      room_id : xxx,
                        *      chess_name: xxxx
                        *   }
                        * */
                        console.log("匹配成功");
                        alert("匹配成功， 您执 " + data.chess_name + "， 黑棋先走");
                        game.room_id = data.room_id;
                        game.my_chess_name = data.chess_name;
                        /*
                        匹配成功后， 持有白棋的人循环获取黑棋落子                        
                        */
                        if(data.chess_name === "白棋"){
                            game.GetPeerStep();
                        }
                        game.match_flag = false;
                        game.start_match.style.background = '#d0cdcd';
                        game.start_match.style.color = '#505050';
                    }
                    else if(data.status === 0 && s >= 30){
                        // 没有匹配上，大于30次弹窗提示"匹配人数太少"
                        alert("当前匹配人数太少，请重新匹配...");
                    }else if(data.status === -1){
                        alert("当前会话失效，请重新登录！");
                        window.location.href = "index.html";
                    }
                },
            });
            if(s >= 30 || game.match_status === 1 || game.match_status === -1){
                clearInterval(_time);
            }
        }, 1000);
    },

    /*
    向后台询问是否是自己的轮回
    */
    IsMyTurn:function (){
        let flag = false;

        let user_info =  {
            room_id : game.room_id,
            user_id : game.user_id
        };

        $.ajax({
            url: "/IsMyTurn",
            type: "Post",
            data: JSON.stringify(user_info),
            dataType: "JSON",
                //async : false ==> 禁止异步请求   chrome 有可能会禁止 ajax 的同步请求
            async: false,
            success: function (data) {
                if (data.status === 1) {
                     flag = true;
                } else {
                     flag = false;
                }
            },
        });
        return flag;
    },

    Step:function (){
        let peer_step_json = JSON.parse(game.peer_step);
        console.log(peer_step_json)

        let dx = peer_step_json.x;
        let dy = peer_step_json.y;

        let WBobj = {
            ox: (dx * 30) - 25,
            oy: (dy * 30) - 25,
            mz: game.chess_Name[game.e % 2],
            dm: document.createElement("div"),
            class: game.e % 2 == 1 ? "Wchess" : "Bchess",
            list: game.um++,
            user_id : game.user_id,
            room_id : game.room_id
        }

        game.h.push(WBobj)
            WBobj.dm.classList.add(WBobj.class);
            WBobj.dm.style.left = WBobj.ox + "px";
            WBobj.dm.style.top = WBobj.oy + "px";
            game.chess_Board[dx - 1][dy - 1] = game.chess_Name[game.e % 2];
            game.lianz[dx - 1][dy - 1] = WBobj.dm;
            game.win(dx - 1, dy - 1, game.chess_Name[game.e % 2], game.winXY[0], game.e % 2)
            game.win(dx - 1, dy - 1, game.chess_Name[game.e % 2], game.winXY[1], game.e % 2)
            game.win(dx - 1, dy - 1, game.chess_Name[game.e % 2], game.winXY[2], game.e % 2)
            game.win(dx - 1, dy - 1, game.chess_Name[game.e % 2], game.winXY[3], game.e % 2)
            game.cName.innerText = game.e % 2 == 0 ? game.chess_Name[1] + "走" : game.chess_Name[0] + "走";
            game.sChesee.className = game.e % 2 == 1 ? "state-chess Bchess" : "state-chess Wchess";
            game.e++;
            game.centent.appendChild(WBobj.dm)
    },

    GetPeerStep: function (){
        /*
        循环获取对方落子位置，直到拿到
        1. 未拿到循环获取 2. 拿到显示到棋盘上
        */
        let _time;
        let user_info = {
            room_id : game.room_id,
            user_id : game.user_id
        }

        let s = 0;
        let status = -100;

        _time = setInterval(() => {
          
            console.log("轮询获取对方棋子位置: " + s++ );
            $.ajax({
           
            url: "/GetPeerStep",
            type: "Post",
            data: JSON.stringify(user_info),
            dataType: "JSON",
            async: false,
            success: function (data) {
                /*
                    data.status : 有没有获取成功
                    1. 获取成功
                    data.peer_step : 对端位置
                */
                status = data.status;
                if (data.status === 1) {
                    console.log("peer_step:" + data.peer_step)
                    game.peer_step = data.peer_step;
                    /*
                    显示对端落子
                    */
                    game.Step();
                }else if(data.status === -1){
                    window.location.href("index.html")
                }else if(data.status === 0){
                    console.log("未获取到，继续获取");
                }
            },
        });

        if(status === 1 || s >= 60 || status === -1){
            clearInterval(_time);
        }
    }, 1000);
},



    canvasClick: function (e) {
        let dx = parseInt(Math.floor(e.offsetX + 15) / 30);
        let dy = parseInt(Math.floor(e.offsetY + 15) / 30);

        if (dx < 1 || dx > 14 | dy < 1 || dy > 14)
            return;

        if(game.IsMyTurn() === false) {
            return;
        }

        let WBobj = {
            x : dx,
            y : dy,
            ox: (dx * 30) - 25,
            oy: (dy * 30) - 25,
            mz: game.chess_Name[game.e % 2],
            dm: document.createElement("div"),
            class: game.e % 2 == 1 ? "Wchess" : "Bchess",
            list: game.um++,
            user_id : game.user_id,
            room_id : game.room_id
        }


        if (game.chess_Board[dx - 1][dy - 1] == 0) {

            $.ajax({
                url: "/Step",
                type: "Post",
                data: JSON.stringify(WBobj),
                dataType: "JSON",
                async: false,
                success: function (data) {
                    /*
                        data.status : 有没有获取成功
                        1. 获取成功
                        data.peer_step : 对端位置
                    */
                    status = data.status;
                    if (data.status === 1) {
                        console.log("落子成功" + data.peer_step)
                        /*
                        获取对方棋子
                        */
                        game.GetPeerStep();
                    }else if(data.status === -1){
                        console.log("会话失效，请重新登录...");
                    }else if(data.status === 0){
                        console.log("等待对方落子");
                    }
                },
            });

            game.h.push(WBobj)
            WBobj.dm.classList.add(WBobj.class);
            WBobj.dm.style.left = WBobj.ox + "px";
            WBobj.dm.style.top = WBobj.oy + "px";
            game.chess_Board[dx - 1][dy - 1] = game.chess_Name[game.e % 2];
            game.lianz[dx - 1][dy - 1] = WBobj.dm;
            game.win(dx - 1, dy - 1, game.chess_Name[game.e % 2], game.winXY[0], game.e % 2)
            game.win(dx - 1, dy - 1, game.chess_Name[game.e % 2], game.winXY[1], game.e % 2)
            game.win(dx - 1, dy - 1, game.chess_Name[game.e % 2], game.winXY[2], game.e % 2)
            game.win(dx - 1, dy - 1, game.chess_Name[game.e % 2], game.winXY[3], game.e % 2)
            game.cName.innerText = game.e % 2 == 0 ? game.chess_Name[1] + "走" : game.chess_Name[0] + "走";
            game.sChesee.className = game.e % 2 == 1 ? "state-chess Bchess" : "state-chess Wchess";
            game.e++;
            game.centent.appendChild(WBobj.dm)
        }

    },

    regret: function (e) {
        if (game.chessOff) {
            if (game.h.length > 0) {
                let obj = game.h.pop();
                let rmRm = obj.dm;
                rmRm.remove()
                game.cName.innerText = game.e % 2 == 0 ? game.chess_Name[1] + "走" : game.chess_Name[0] + "走";
                game.sChesee.className = game.e % 2 == 1 ? "state-chess Bchess" : "state-chess Wchess";
                game.e -= 1;
                game.um -= 1;
                game.chess_Board[parseInt(obj.ox / 30)][parseInt(obj.oy / 30)] = 0;

            } else {
                return;
            }
        } else {

            return;
        }

    },

    anewClick: function (e) {
        // 1. 先将匹配标注为设置为true
        game.match_flag = true;
        // 1.服务端收到重开后，先将服务端房间重置
        // 2.重新整理用户信息
      let user_info = {
          user_id : game.user_id,
          room_id : game.room_id
      };
        
            $.ajax({
                url: "/Restart",
                type: "Post",
                data: JSON.stringify(user_info),
                dataType: "JSON",
                async: false,
                success: function (data) {
                    /*
                        data.status : 有没有获取成功
                        0: 获取成功
                        -1 : 失败
                    */
                    status = data.status;
                    if (data.status === 0) {
                        console.log("重置成功");
                        game.SetMatch();
                      }
                },
            });
        game.h.forEach(function (v, i) {
            v.dm.remove()
            game.h = []
            game.um = 0;
            game.chessOff = true;
        })
        for (var i = 0; i <= 13; i++) {
            game.chess_Board[i] = [];
            game.lianz[i] = [];
            for (var j = 0; j <= 13; j++) {
                game.chess_Board[i][j] = 0;
                game.lianz[i][j] = 0;
            }
        }
        game.winName.innerText = ' ';
        game.winner.style.display = "none";
        game.regret_chess.style.background = '';
        game.regret_chess.style.color = '';
    },

    win: function (x, y, c, m, li) {
        let ms = 1;
        var continuity = [];
        for (let i = 1; i < 5; i++) {
            if (game.chess_Board[x + i * m[0]]) {
                if (game.chess_Board[x + i * m[0]][y + i * m[1]] === c) {
                    continuity.push([x + i * m[0], y + i * m[1]])
                    ms++;
                } else {
                    break;
                }
            }
        }

        for (let i = 1; i < 5; i++) {
            if (game.chess_Board[x - i * m[0]]) {
                if (game.chess_Board[x - i * m[0]][y - i * m[1]] === c) {
                    continuity.push([x - i * m[0], y - i * m[1]])
                    ms++;
                } else {
                    break;
                }
            }
        }

        if (ms >= 5) {
            if(c === game.my_chess_name)
            {
                let winner_info = {
                    user_id : game.user_id,
                    room_id : game.room_id
                };
                
            $.ajax({
                url: "/Winner",
                type: "Post",
                data: JSON.stringify(winner_info),
                dataType: "JSON",
                async: false,
                success: function (data) {
                    /*
                        data.status : 有没有获取成功
                        0: 获取成功
                        -1 : 失败
                    */
                    status = data.status;
                    if (data.status === 0) {
                        console.log("发送胜者成功")
                      }
                },
            });
            }
            //alert(c + "赢了")
            setTimeout(function () {
                console.log(c + "赢了")
            }, 600)
            continuity.push([x, y]);
            game.chessOff = false;
            game.regret_chess.style.background = '#d0cdcd';
            game.regret_chess.style.color = '#505050';
            let s = 5;
            let ls = [270, 300, 330, 360, 390];
            let ls1 = [390, 420, 450, 480, 510];
            continuity.forEach(function (value, index) {
                let time = setInterval(function () {
                    game.lianz[value[0]][value[1]].style.transform = 'scale(0.9)';
                    game.lianz[value[0]][value[1]].style.boxShadow = "0px 0px 2px 2px #ffd507";
                    s--;
                    s <= 0 ? clearInterval(time) : clearInterval(time);
                }, ls[index])
                let time2 = setInterval(function () {
                    game.lianz[value[0]][value[1]].style.transform = 'scale(1)';
                    game.lianz[value[0]][value[1]].style.boxShadow = "0px 0px 2px 2px #ffd507";
                    s++
                    s >= 5 ? clearInterval(time2) : clearInterval(time2);
                }, ls1[index])
            })

            for (var i = 0; i < game.chess_Board.length; i++) {
                for (var j = 0; j < game.chess_Board.length; j++) {
                    if (game.chess_Board[i][j] === 0) {
                        game.chess_Board[i][j] = "null";
                    }
                }
            }

            game.h.forEach(function (value, index) {
                value.dm.innerText = value.list;
            })

            li == 1 ? game.winChesee.className = "state-chess Wchess" : game.winChesee.className = "state-chess Bchess";
            game.winName.innerText = c + "赢了";
            this.winner.style.display = "block";
        }
    },

};

