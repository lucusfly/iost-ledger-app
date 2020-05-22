// const Transport = require('@ledgerhq/hw-transport');
const TransportU2F = require( "@ledgerhq/hw-transport-u2f");
const TransportNodeHid = require( "@ledgerhq/hw-transport-node-hid-noevents");
const Logs = require( "@ledgerhq/logs");
const Utils = require("./utils");


module.exports = function(appName, transport) {
    // global.fetch = () => {
    //     console.error('required node version >= 10.x')
    //     process.exit(10);
    // };
  const BIP32_PATH = "44'/291'";
  const EXCHANGE_CHUNK_SIZE = 255;
  const APDU = {
    CLA: 0xE0,
    INS_GET_CONFIGURATION: 0x02,
    INS_GET_PUBLIC_KEY: 0x04,
    INS_SIGN_MESSAGE: 0x08,
  };
  const sendApdu = async ({instruction, p1, p2, index, data}) => {
    let buffer = Utils.bufferFromBip32(BIP32_PATH);

    if (p1 === undefined) {
      p1 = Utils.debugOrNot(this.P1P2.SILENT, this.P1P2.CONFIRM);
    }
    if (p2 === undefined) {
      p2 = this.P1P2.BIN;
    }
    if (index !== undefined) {
      buffer = Utils.bufferFromBip32(BIP32_PATH + "/" + index + "'");
    }

    if (data instanceof Buffer) {
      buffer = Buffer.concat([
        buffer,
        data
      ]);
    }

    data = Buffer.alloc(0);

    for (
      let offset = 0, chunk = buffer;
      data.length === 0;
      offset += chunk.length, chunk = buffer
    ) {
      if ((p2 & this.P1P2.MORE) == this.P1P2.MORE) {
        chunk = chunk.slice(offset, offset + EXCHANGE_CHUNK_SIZE);
      }

      if (Utils.debugOrNot(true, false)) {
        console.log("instruction:", instruction, ", p1: ", p1, ", p2: ", p2, "chunk: ", chunk);
      }

      data = await this.transport.send(
        APDU.CLA,
        instruction,
        p1,
        p2,
        chunk
      );

      if (data.length === 2 && Utils.arrayToWord(data) == 0x9000) {
        data = Buffer.alloc(0);
      }
    }

    const truncate = (asUtf8, size) => {
      let result = data.slice(0, 0 - (size || 3));
      if (asUtf8) {
        result = result.toString("utf8");
      }
      return result;
    };

    switch (p2 & (~this.P1P2.MORE)) {
    case this.P1P2.HEX:
      data = {hex: truncate(true)};
      break;
    case this.P1P2.BASE58:
      data = {base58: truncate(true)};
      break;
    case this.P1P2.BIN:
      data = {bin: truncate(false)};
      break;
    }
    return data;
  };

  if (
    Utils.debugOrNot(true, false) &&
    process.env.DEBUG === "1"
  ) {
    Logs.listen(console.log);
  }

  this.P1P2 = {
    CONFIRM: 0x00,
    SILENT: 0x01,
    BIN: 0x00,
    HEX: 0x01,
    BASE58: 0x02,
    MORE: 0x80
  };
  this.open = async () => {
    let t = null;
    let descriptor = "";

    switch (transport.split(":")[0]) {
      case "u2f":
        t = await TransportU2F.default;
        break;
      case "hid":
        t = await TransportNodeHid.default;
        break;
      }

    if (t === null) {
      throw new Error("Unknown ledger transport - " + transport);
    } else if (! await t.isSupported()) {
      throw new Error("Ledger transport '" + transport + "' not supported on this platform");
    } else {
      this.transport = await t.open(descriptor);
      this.transport.decorateAppAPIMethods(
        this,
        [
          "getConfiguration",
          "getPublicKey",
          "signTransaction"
        ],
        appName
      );
    }
  };
  this.close = async () => {
    await this.transport.close();
  };
  this.getConfiguration = async () => {
    const conf = await this.transport.send(
      APDU.CLA,
      APDU.INS_GET_CONFIGURATION,
      Utils.debugOrNot(this.P1P2.SILENT, this.P1P2.CONFIRM),
      this.P1P2.BIN,
      Utils.bufferFromBip32(BIP32_PATH)
    );
    return {
      hasStorage: conf[0] != 0,
      version: conf[1] + "." + conf[2] + "." + conf[3]
    }
  };
  this.getPublicKey = async ({index, p1, p2}) => {
    return await sendApdu({instruction: APDU.INS_GET_PUBLIC_KEY, p1, p2, index});
  };
  this.signMessage = async ({index, p1, p2, message}) => {
    const data = message;
    return await sendApdu({instruction: APDU.INS_SIGN_MESSAGE, p1, p2, index, data});
  };
};
