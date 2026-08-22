const fs = require('fs');

const files = [
  'firmware/esp32/web/desktop/calculator.html',
  'firmware/esp32/web/mobile/calculator.html',
];

for (const file of files) {
  const text = fs.readFileSync(file, 'utf8');
  const fail = (message) => {
    console.error(`${file}: ${message}`);
    process.exitCode = 1;
  };

  if (!text.includes('id="sourceWires"')) fail('single source wire input is missing');
  if (!text.includes("split(';')")) fail('semicolon-separated wire parser is missing');
  if (!text.includes('raw.length>5')) fail('five-wire UI limit is missing');
  if (!text.includes('source_component_count')) fail('multi-component request count is missing');
  if (!text.includes("source_diameter_'")) fail('multi-component diameter args are missing');
  if (!text.includes("source_strands_'")) fail('multi-component strand args are missing');
  if (!text.includes("q.set('source_strands_'+n,'1')")) fail('each entered diameter must represent one source wire');
  if (text.includes('id="diameter"') || text.includes('id="strands"')) {
    fail('legacy diameter/strand input pair must not return');
  }
}

if (!process.exitCode) {
  console.log('Calculator source wire input contracts OK');
}
