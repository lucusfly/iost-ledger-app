const path = require('path');

module.exports = {
    mode: 'production',
    entry: './contract.js',
    output: {
        library: 'contractCompile',
        libraryTarget: 'commonjs2',
        path: path.resolve(__dirname, 'dist'),
        filename: 'bundle.js'
    }
};