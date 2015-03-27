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
  var readBannerBytes = 0
  var bannerLength = 2
  var readFrameBytes = 0
  var frameBodyLength = 0
  var frameBody = new Buffer(0)
  var banner = {
    version: 0
  , length: 0
  , realWidth: 0
  , realHeight: 0
  , virtualWidth: 0
  , virtualHeight: 0
  , orientation: 0
  , quirks: 0
  }

  function tryRead() {
    for (var chunk; (chunk = stream.read());) {
      console.info('chunk(length=%d)', chunk.length)
      for (var cursor = 0, len = chunk.length; cursor < len;) {
        if (readBannerBytes < bannerLength) {
          switch (readBannerBytes) {
          case 0:
            // version
            banner.version = chunk[cursor]
            break;
          case 1:
            // length
            banner.length = bannerLength = chunk[cursor]
            break;
          case 2:
          case 3:
          case 4:
          case 5:
            // real width
            banner.realWidth += chunk[cursor] << ((readBannerBytes - 2) * 8)
            break;
          case 6:
          case 7:
          case 8:
          case 9:
            // real height
            banner.realHeight += chunk[cursor] << ((readBannerBytes - 6) * 8)
            break;
          case 10:
          case 11:
          case 12:
          case 13:
            // virtual width
            banner.virtualWidth += chunk[cursor] << ((readBannerBytes - 10) * 8)
            break;
          case 14:
          case 15:
          case 16:
          case 17:
            // virtual height
            banner.virtualHeight +=
              chunk[cursor] << ((readBannerBytes - 14) * 8)
            break;
          case 18:
            // orientation
            banner.orientation += chunk[cursor] * 90
            break;
          case 19:
            // quirks
            banner.quirks = chunk[cursor]
            console.log('banner', banner)
            break;
          }
          cursor += 1
          readBannerBytes += 1
        }
        else if (readFrameBytes < 4) {
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
