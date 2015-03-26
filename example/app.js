var WebSocketServer = require('ws').Server
  , http = require('http')
  , express = require('express')
  , path = require('path')
  , net = require('net')
  , app = express()

var PORT = process.env.PORT || 9002

app.use(express.static(path.join(__dirname, '/public')))

var server = http.createServer(app)
var wss = new WebSocketServer({ server: server })

wss.on('connection', function(ws) {
  console.info('Got a client')

  var stream = net.connect({ port: 1717 })
  var readFrameBytes = 0
  var frameBodyLength = 0
  var frameBody = new Buffer(0)

  function tryRead() {
    for (var chunk; (chunk = stream.read());) {
      console.info('chunk(length=%d)', chunk.length)
      for (var cursor = 0, len = chunk.length; cursor < len;) {
        if (readFrameBytes < 4) {
          frameBodyLength += chunk[cursor] << (readFrameBytes * 8)
          cursor += 1
          readFrameBytes += 1
          console.info('headerbyte%d(val=%d)', readFrameBytes, frameBodyLength)
        }
        else {
          if (len - cursor >= frameBodyLength) {
            console.info('bodyfin(len=%d,cursor=%d)', frameBodyLength, cursor)

            frameBody = Buffer.concat([
              frameBody
            , chunk.slice(cursor, cursor + frameBodyLength)
            ])

            ws.send(frameBody, {
              binary: true
            })

            cursor += frameBodyLength
            frameBodyLength = readFrameBytes = 0
            frameBody = new Buffer(0)
          }
          else {
            console.info('body(len=%d)', len - cursor)

            frameBody = Buffer.concat([
              frameBody
            , chunk.slice(cursor, len)
            ])

            frameBodyLength -= len - cursor
            readFrameBytes += len - cursor
            cursor = len
          }
        }
      }
    }
  }

  stream.on('readable', tryRead)

  ws.on('close', function() {
    console.info('Lost a client')
    stream.end()
  })
})

server.listen(PORT)
console.info('Listening on port %d', PORT)
