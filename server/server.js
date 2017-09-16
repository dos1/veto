/* PROTOCOL
 broadcasted events:
   S - game starts
   V - voting starts
   C{1} - voting counter, {1} secs left
   F{1} - {1} players voted for
   A{1} - {1} players voted against
   N{1} - {1} players didn't vote
   E{1} - voting ended; {1} is a result: either F (for) or A (against)
      
   v{1} - player with nick {1} used veto
   T{1} - the end of the game, because of veto from player {1}
   W0{1} - {1} is the winner of the best deputy award
   W1{1} - {1} is the veto 1st winner
   W2{1} - {1} is the veto 2nd winner
   W3{1} - {1} is the veto 3rd winner
   
 events sent only to monitor:
   P{1} - currently {1} players are connected

   J{1} - player with nick {1} joined
   L{1} - player with nick {1} left
   R{1} - player with nick {1} reconnected

 events sent only to players:
   cookie:{1} - cookie for reconnecting
   nick:{1} - nick sent after reconnecting
   end - this player has ended their game
   score:{1} - current score of the player
   canVeto - player can veto in this round
   err - if reconnection didn't succeed
   ok - if reconnection did succeed
   
--------------------------------------------------
monitor can send:
  {"type": "monitor"} - to become a monitor (there can be only one)
  {"type": "start"} - to start the game
  {"type": "voting"} - to start the voting
player can send:
  {"type": "join", "nick": string} - to join
  {"type": "reconnect", "cookie": string} - to reconnect
  {"type": "vote", "choice": choice} - to vote. can be changed before the voting ends; choice: true/false/null
  {"type": "veto"} - to veto.
  
debug:
  TODO: {"type": "debug"} - returns data of all players and state
  
TODO: test reconnect, start vs. voting; implement veto and debug monitor
*/

const VOTING_TIME = 5;

const WebSocket = require('ws');
const randomstring = require('randomstring');

const wss = new WebSocket.Server({ port: 8889 });

let players = [];

let state = {
  monitor: null,
  voting: false,
  counter: null,
  started: false,
  round: 0,
  canVeto: false,
  vetos: 0,
  vetoers: []
};

let countVotes = function() {
    let allplayers = 0;
    wss.clients.forEach(function each(client) {
      if (client.readyState === WebSocket.OPEN) {
        if (client.data) {
          allplayers++;
        }
      }
    });


    let forvotes = 0; let against = 0;
    players.forEach(function(player) {
      if (player.vote == true) {
        forvotes++;
      }
      if (player.vote == false) {
        against++;
      }
    });
    let nonevotes = allplayers - forvotes - against;

    wss.broadcast('F' + forvotes);
    wss.broadcast('A' + against);
    wss.broadcast('N' + (nonevotes - state.vetoers.length));
 
    let result = 'A';
    if (forvotes > against) {
      result = 'F'
    }

    wss.broadcast('E' + result);

    players.forEach(function(player) {
      if (player.vote == (result == 'F')) { // points for voting like majority
        player.score++;
      } else if (player.vote != null) {
        player.score--;
      }
      if (player.ws.readyState == WebSocket.OPEN) {
        player.ws.send('score:'+player.score);
      }

    });
    
    state.round++;

};

let startGame = function() {
  if (state.started) {
    console.log('RESTART');
  } else {
    console.log('START');
  }
  state.started = true;
  
  state.vetoers = [];
  state.counter = 0;
  state.voting = false;
  state.vetos = 0;
  state.canVeto = false;
  
  players.forEach(function(player) {
      player.vote = null;
      player.vetoRight = false;
      player.score = 0;
      player.ended = false;
      if (player.ws.readyState == WebSocket.OPEN) {
        player.ws.send('score:0');
      }
  });
  
  state.round=1;
  
  wss.broadcast('S');

};

function dynamicSort(property) {
    var sortOrder = 1;
    if(property[0] === "-") {
        sortOrder = -1;
        property = property.substr(1);
    }
    return function (a,b) {
        var result = (a[property] < b[property]) ? -1 : (a[property] > b[property]) ? 1 : 0;
        return result * sortOrder;
    }
}

let startVote = function() {
    
    if (state.voting) return;
    
    console.log('VOTE round ' + state.round);

    if (state.round % 5 == 0) {
        state.canVeto = true;
    } else {
        state.canVeto = false;
    }
    
  players.forEach(function(player) {
    player.vote = null;
    player.vetoRight = false;
  });
  
  state.voting = true;
  wss.broadcast('V');

  let splayers = players.sort(dynamicSort("score")).filter(function(user) {
      return !user.ended;
  });
  
  if (state.canVeto) {
    if (splayers[2]) {
        let i = 2;
        while ((splayers[i]) && (splayers[i].score == splayers[2].score)) {
            splayers[i].vetoRight = true;
            if (splayers[i].ws.readyState == WebSocket.OPEN) {
              splayers[i].ws.send('canVeto');
            }
            i++;
        }
    }
    if (splayers[1]) {
        splayers[1].vetoRight = true;
            if (splayers[1].ws.readyState == WebSocket.OPEN) {
            splayers[1].ws.send('canVeto'); }
    }
    if (splayers[0]) {
        splayers[0].vetoRight = true;
            if (splayers[0].ws.readyState == WebSocket.OPEN) {
            splayers[0].ws.send('canVeto');}
    }
  }

  state.counter = VOTING_TIME + 1;

  var tick = function() {
      if (!state.voting) return;
    if (state.counter > 0) {
      state.counter--;
      wss.broadcast('C' + state.counter);
      setTimeout(tick, 1000);
    } else {
      state.voting = false;
      countVotes();
    }
  };

  tick();
};

// Broadcast to all.
wss.broadcast = function broadcast(data) {
  wss.clients.forEach(function each(client) {
    if (client.readyState === WebSocket.OPEN) {
      if ((client.data) || (client == state.monitor)) {
        if ((!client.data) || (!client.data.ended)) {
            client.send(data);
    }
      }
    }
  });
};

wss.on('connection', function connection(ws) {

  //wss.broadcast("connected");

  ws.on('close', function() {
    if (ws==state.monitor) {
      state.monitor = null;
      return;
    }
    if (!ws.data) return;
    let playerCount = 0;
    ws.data.connected = false;
    wss.clients.forEach(function each(client) {
      if (client.readyState === WebSocket.OPEN) {
        if ((client.data) && (client.data.connected)) {
          playerCount++;
        }
      }
    });
    if (state.monitor) {
      state.monitor.send('P' + playerCount);
      state.monitor.send('L' + ws.data.name);
    }
  });

  ws.on('message', function incoming(data) {
    // Broadcast to everyone.
    //wss.broadcast(data);

    console.log('got ' + data + ' from ' + (ws.data ? ws.data.name : null))

    //if (data.startsWith('{')) {
    try {
      data = JSON.parse(data);
    } catch(e) {
      console.log('error parsing ' + data + ' - ' + (ws.data ? ws.data.name : null))
      return;
    }
    //}

    if (data.type == 'monitor') {
      state.monitor = ws;
      console.log('monitor registered');
      
     let playerCount = 0;
      wss.clients.forEach(function each(client) {
        if (client.readyState === WebSocket.OPEN) {
          if ((client.data) && (client.data.connected)) {
            playerCount++;
          }
        }
      });
    state.monitor.send('P' + playerCount);

    }

    if (data.type == 'join') {
      if (ws.data) return;
      ws.data = {
        name: data.nick.trim(),
        score: 0,
        vote: null,
        vetoRight: false,
        ended: false,
        cookie: randomstring.generate(),
        connected: true,
        ws: ws
      };
      players.push(ws.data);
      console.log(ws.data.name + ' joined');

      let playerCount = 0;
      wss.clients.forEach(function each(client) {
        if (client.readyState === WebSocket.OPEN) {
          if ((client.data) && (client.data.connected)) {
            playerCount++;
          }
        }
      });
      if (state.monitor) {
        state.monitor.send('P' + playerCount);
        state.monitor.send('J' + ws.data.name);
      }
      ws.send('cookie:' + ws.data.cookie);
    }

    if (data.type == 'reconnect') {
      // TODO: FIXME: search for cookie and reattach
      players.forEach(function(player) {
        if (data.cookie == player.cookie) {
          ws.data = player;
        }
      });
      
      if (!ws.data) {
          ws.send('err');
          return;
      } else {
          ws.send('ok');
      }
      ws.data.connected = true;
      ws.data.ws = ws;
        
      let playerCount = 0;
      wss.clients.forEach(function each(client) {
        if (client.readyState === WebSocket.OPEN) {
          if ((client.data) && (client.data.connected)) {
            playerCount++;
          }
        }
      });
      if (state.monitor) {
        state.monitor.send('P' + playerCount);
        state.monitor.send('R' + ws.data.name);
      }
      ws.send('score:' + ws.data.score);
      ws.send('nick:' + ws.data.name);
      if (ws.data.ended) ws.send('end');
    }

    if (data.type == 'vote') {
      if (!ws.data) return;
      if (ws.data.ended) return;
      if (state.voting) {
        console.log(ws.data.name + ' voted ' + data.choice)
        ws.data.vote = data.choice;
      }
    }

    if (data.type == 'veto') {
      if (!ws.data) return;
      if (state.voting) {
        if ((ws.data.vetoRight) && (state.canVeto)) {
          state.canVeto = false;
          console.log(ws.data.name + ' said VETO!')
          state.voting = false;
          state.vetos++;
          state.round++;
          ws.data.score -= (VOTING_TIME - state.counter) * 2;
          state.vetoers.push(ws.data);
          ws.send('score:'+ws.data.score);
          ws.send('end');
          ws.data.ended = true;
          
          if (state.vetos == 3) {
              
            players.sort(dynamicSort('-score'));
            
            var bestscore = players[0].score;
            let i = 1;
            let thebests = [players[0].name];
            while (players[i].score == bestscore) {
                thebests.push(players[i].name);
                i++;
            }
            
            wss.broadcast('W0' + thebests.join(', ') + ' ('+bestscore+')');

            state.vetoers.sort(dynamicSort('score'));
            
            wss.broadcast('W1' + state.vetoers[0].name + ' ('+state.vetoers[0].score+')');
            wss.broadcast('W2' + state.vetoers[1].name + ' ('+state.vetoers[1].score+')');
            wss.broadcast('W3' + state.vetoers[2].name + ' ('+state.vetoers[2].score+')');
            

            wss.broadcast('T' + ws.data.name);
            
            console.log("THE END");
          } else {
            wss.broadcast('v' + ws.data.name);
          }

        }
      }
    }

    if (data.type == 'start') {
      if (ws == state.monitor) {
        startGame();
      }
    }
    if (data.type == 'voting') {
      if (ws == state.monitor) {
        startVote();
      }
    }

    if (data.type == 'debug') {
ws.send(JSON.stringify(players, function(key, value) {
    if (key==='ws') return;
    return value;
}));

ws.send(JSON.stringify(state, function(key, value) {
    if (key==='monitor') return;
    if (key==='ws') return;
    return value;
}));


    }

  });
});
