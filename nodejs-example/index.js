const Iost = require("./lib/iost");
const Ledger = require("./lib/ledger");
const Utils = require("./lib/utils");
const Config = require("config");
const Assert = require("assert");

const iost = new Iost(Config.iost.api.url, Config.iost.api.chain);
const ledger = new Ledger("IOST", Config.ledger.transport);
const test = {
  checkTrx: (trx, chainId) => {
    Assert.strictEqual(trx.gasRatio, 1);
    Assert.strictEqual(trx.gasLimit, 1000000);
    Assert.deepEqual(trx.signers, []);
    Assert.deepEqual(trx.signatures, []);
    Assert.strictEqual(trx.chain_id, chainId || 1024);
    Assert.strictEqual(trx.reserved, null);
    Assert.strictEqual(trx.delay, 0);
    Assert.strictEqual(trx.expiration - trx.time, 90000000000);
    Assert.deepStrictEqual(trx.amount_limit, [iost.amountOf(20)]);
  },
  checkCommit: (trx, commit) => {
    test.checkTrx(trx, 1023);
    Assert.strictEqual(commit.status_code, iost.STATUS_CODE_SUCCESS);
    Assert.strictEqual(commit.message, "");
    Assert.strictEqual(iost.base58ToArray(commit.tx_hash).length, 32);
    Assert.deepStrictEqual(commit.returns, ['[""]', '[]']);
  },
  checkTransfer: (commit) => {
    Assert.strictEqual(commit.status_code, iost.STATUS_CODE_SUCCESS);
    Assert.strictEqual(commit.message, "");
    Assert.deepStrictEqual(commit.returns, ['[]']);
    Assert.strictEqual(commit.receipts.length, 1);
    Assert.strictEqual(commit.receipts[0].func_name, 'token.iost/transfer');
  },
  createTrxAction: (action, actionName, contract) => {
    contract = contract || iost.TRX_CONTRACT_AUTH;
    return {
      contract,
      actionName,
      data: JSON.stringify(action).replace(' ', '')
    };
  },
  createTrxReceipt: (receipt, funcName) => {
    return {
      func_name: funcName,
      data: JSON.stringify(receipt).replace(' ', '')
    };
  },
  createTrxSignupAction: (action) => test.createTrxAction(action, iost.TRX_METHOD_SIGNUP),
  createTrxSignupReceipt: (receipt) => test.createTrxReceipt(receipt, iost.TRX_CONTRACT_AUTH + "/" + iost.TRX_METHOD_SIGNUP),
  createTrxTransferAction: (action) => test.createTrxAction(action, iost.TRX_METHOD_SIGNUP),
  createTrxTransferReceipt: (receipt) => test.createTrxReceipt(receipt, iost.TRX_CONTRACT_AUTH + "/" + iost.TRX_METHOD_TRANSFER),
  getTrxSignupAction: (txInfo) => txInfo.findInActions(iost.TRX_METHOD_SIGNUP),
  getTrxSignupReceipt: (txInfo) => txInfo.findInReceipts(iost.TRX_METHOD_SIGNUP),
  getTrxTransferAction: (txInfo) => txInfo.findInActions(iost.TRX_METHOD_TRANSFER),
  getTrxTransferReceipt: (txInfo) => txInfo.findInReceipts(iost.TRX_METHOD_TRANSFER)
};

/*****************************************************************************************************
 * First we use predefined account 'ledger_001' with provided private key to create new account A. (Lines 73 - 85)
 * This account uses newly generated key pair and dosn't store keys on ledger.
 * Then we retrieve the public key from ledger and create new account B owned by account A. (Lines 86 - 102)
 * And finally we create account C owned by account B and sign transaction with ledger. (Lines 103 - 149)
 *****************************************************************************************************/

Utils.delay().then(ledger.open).then(async () => {
  //! Check ledger app version
  const ledgerConfig = await ledger.getConfiguration();
  Assert.deepStrictEqual(ledgerConfig, {hasStorage: false, version: "0.0.3"});

  //! Create new account A with software keys
  let account = iost.login(Config.iost.account.name, Config.iost.account.key);
  const keyPairA = iost.createKeyPair();
  const pubKeyA = keyPairA.id;
  const trxA = iost.newAccount("a", account.getID(), pubKeyA, 1, 10);
  const trxInfoA = test.getTrxSignupAction(iost.getTxInfo(trxA));

  test.checkTrx(trxA);
  Assert.strictEqual(trxInfoA[1], pubKeyA);
  Assert.strictEqual(trxInfoA[2], pubKeyA);
  Assert.deepStrictEqual(trxA.actions[0], test.createTrxSignupAction(trxInfoA));
  console.log("Ready to create account A");

  //! Complete A creation by master and create new account B with hardware keys
  const commitA = await iost.commitTx(trxA, account);
  const pubKeyB = await ledger.getPublicKey({index: 0, p2: ledger.P1P2.BASE58});
  const trxB = iost.newAccount("b", trxInfoA[0], pubKeyB.base58, 1, 10);
  const trxInfoB = test.getTrxSignupAction(iost.getTxInfo(trxB));

  test.checkCommit(trxA, commitA);
  Assert.deepStrictEqual(trxInfoA, test.getTrxSignupReceipt(iost.getTxInfo(commitA)));
  test.checkTrx(trxB);
  Assert.deepStrictEqual(trxB.actions[0], test.createTrxSignupAction(trxInfoB));
  Assert.strictEqual(trxInfoB[1], pubKeyB.base58);
  Assert.strictEqual(trxInfoB[2], pubKeyB.base58);
  console.log("Ready to create account B");

  //! Transfer some coins from master to A and switch to account A
  test.checkTransfer(await iost.commitTx(iost.newTranfer(account.getID(), trxInfoA[0], 88), account));
  account = iost.makeAccount(trxInfoA[0], keyPairA);

  //! Complete B creation by A and create new account C with hardware keys
  const commitB = await iost.commitTx(trxB, account);
  const pubKeyC = await ledger.getPublicKey({index: 1, p2: ledger.P1P2.BASE58});
  const trxC = iost.newAccount("c", trxInfoB[0], pubKeyC.base58, 1, 10);
  const trxInfoC = test.getTrxSignupAction(iost.getTxInfo(trxC));

  test.checkCommit(trxB, commitB);
  Assert.deepStrictEqual(trxInfoB, test.getTrxSignupReceipt(iost.getTxInfo(commitB)));
  test.checkTrx(trxC);
  Assert.deepStrictEqual(trxC.actions[0], test.createTrxSignupAction(trxInfoC));
  Assert.strictEqual(trxInfoC[1], pubKeyC.base58);
  Assert.strictEqual(trxInfoC[2], pubKeyC.base58);
  console.log("Ready to create account C");

  //! Transfer some coins from A to B and switch to account B
  test.checkTransfer(await iost.commitTx(iost.newTranfer(account.getID(), trxInfoB[0], 66), account));
  account = {
    name: trxInfoB[0],
    pubKey: pubKeyB.base58,
    sign: async (bytes) => {
      return await ledger.signMessage({
        index: 0,
        message: iost.createHash(bytes)
      });
    }
  };

  //! Complete C creation by B
  const commitC = await iost.commitTx(trxC, account);
  test.checkCommit(trxC, commitC);
  Assert.deepStrictEqual(trxInfoC, test.getTrxSignupReceipt(iost.getTxInfo(commitC)));
  console.log("Ready to transfer funds");

  //! Transfer some coins from B to C and switch to account C
  test.checkTransfer(await iost.commitTx(iost.newTranfer(account.name, trxInfoC[0], 33), account));
  account = {
    name: trxInfoC[0],
    pubKey: pubKeyC.base58,
    sign: async (bytes) => {
      return await ledger.signMessage({
        index: 1,
        message: bytes,
        p2: ledger.P1P2.BIN | ledger.P1P2.MORE
      });
    }
  };

  //! Transfer some coins from C to A by signing trx raw bytes
  test.checkTransfer(await iost.commitTx(iost.newTranfer(account.name, trxInfoA[0], 11), account));
  Assert.deepStrictEqual(await iost.getBalance(account.name), {balance: 22, frozen_balances: []});
  console.log("COMPLETE");

}).then(ledger.close).catch(Utils.fail);
