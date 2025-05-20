let timeData = [];
let tempSeries = {
    watertemp: [],
    pooltemp: [],
    composttemp: []
};

const gauges = {};
let poolGaugeRotation = 0;


function createGauge(ctx, label, color = '#ff6666') {
    return new Chart(ctx, {
        type: 'doughnut',
        data: {
            labels: [label],
            datasets: [{
                data: [0, 100],
                backgroundColor: [color, '#eee'],
                borderWidth: 0
            }]
        },
        options: {
            rotation: 0,  // On ajoutera un offset animé ici
            cutout: '80%',
            responsive: true,
            animation: false,  // Important pour contrôler manuellement la rotation
            plugins: {
                tooltip: { enabled: false },
                legend: { display: false },
                title: { display: true, text: label }
            }
        }
    });
}

function updateGauge(chart, value, max = 100) {
    chart.data.datasets[0].data = [value, max - value];
    chart.update();
}

function updateLineChart() {
    lineChart.data.labels = timeData;
    lineChart.data.datasets[0].data = tempSeries.watertemp;
    lineChart.data.datasets[2].data = tempSeries.composttemp;
    lineChart.update();
}

async function fetchAndUpdate() {
    const res = await fetch('/api/temperatures');
    const data = await res.json();

    const t = data.elapsed;

    // Mise à jour des jauges avec affichage de la température/ humidité
    updateGauge(gauges.watertemp, data.watertemp, 50);
    
    updateGauge(gauges.pooltemp, data.rate, 5);

    



    updateGauge(gauges.composttemp, data.composttemp, 70);
    updateGauge(gauges.composthumid, data.composthumid, 100);
    

        let batteryColor = 'green';
    if (data.batterylevel < 20) {
        batteryColor = 'red';
    } else if (data.batterylevel < 70) {
        batteryColor = 'orange';
    } else {
        batteryColor = 'green';
    }
    gauges.batterylevel.data.datasets[0].backgroundColor = [batteryColor, '#eee'];
    updateGauge(gauges.batterylevel, data.batterylevel, 100);

    // Mise à jour des valeurs numériques au centre des gauges
    document.getElementById('value_water').textContent = `${data.watertemp}°C`;
    document.getElementById('value_pool').textContent = `${data.rate}L/min`;
    document.getElementById('value_compost').textContent = `${data.composttemp}°C`;
    document.getElementById('value_humid').textContent = `${data.composthumid}%`;
    document.getElementById('value_battery').textContent = `${data.batterylevel}%`;

    // Avertissement si l'humidité est trop basse
    if (data.composthumid < 40) {
        document.getElementById('warning_humid').textContent = "Humidifiez votre compost";
    } else {
        document.getElementById('warning_humid').textContent = "";
    }

    timeData.push(t);
    tempSeries.watertemp.push(data.watertemp);
    tempSeries.pooltemp.push(data.rate);
    tempSeries.composttemp.push(data.composttemp);

    if (timeData.length > 60) {
        timeData.shift();
        tempSeries.watertemp.shift();
        tempSeries.pooltemp.shift();
        tempSeries.composttemp.shift();
    }

    updateLineChart();
}

let lineChart;

window.onload = () => {
    gauges.watertemp = createGauge(document.getElementById('gauge_water'), 'Eau', 'blue');
    gauges.pooltemp = createGauge(document.getElementById('gauge_pool'), "Debit de l'eau", 'blue');
    gauges.composttemp = createGauge(document.getElementById('gauge_compost'), 'Compost', 'green');
    gauges.composthumid = createGauge(document.getElementById('gauge_humid'), 'Humidité', 'orange');
    gauges.batterylevel = createGauge(document.getElementById('gauge_battery'), 'Batterie', 'yellow');

    const ctx = document.getElementById("lineChart").getContext("2d");
    lineChart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [
                { label: 'Eau', borderColor: 'blue', data: [], fill: false },
                { label: '', borderColor: 'black', data: [], fill: false },
                { label: 'compost', borderColor: 'green', data: [], fill: false }
            ]
        },
        options: {
            animation: false,
            responsive: true,
            scales: {
                y: { title: { display: true, text: '°C' } },
                x: { title: { display: true, text: 'Temps (s)' } }
            }
        }
    });

    setInterval(update, 1000);
};

function startSystem() {
    
    fetch('/api/startpump', { method: 'POST' })
        .then(response => response.json())
        .then(data => {
            console.log('Pompe démarrée :', data);
            const btn1 = document.getElementById('startButton');
            btn1.style.backgroundColor = 'grey';
            btn1.disabled = true;
            const btn2 = document.getElementById('stopButton');
            btn2.style.backgroundColor = 'red';
            btn2.disabled = false;
            
        })
        .catch(error => {
            console.error('Erreur de démarrage :', error);
            alert("Erreur lors du démarrage !");
        });
}

function setSystemToActiv() {
    const btn1 = document.getElementById('startButton');
    btn1.style.backgroundColor = 'grey';
    btn1.disabled = true;
    const btn2 = document.getElementById('stopButton');
    btn2.style.backgroundColor = 'red';
    btn2.disabled = false;
}

function setSystemToInnactiv() {
    const btn1 = document.getElementById('startButton');
    btn1.style.backgroundColor = 'green';
    btn1.disabled = false;
    const btn2 = document.getElementById('stopButton');
    btn2.style.backgroundColor = 'grey';
    btn2.disabled = true;
}

async function fetchPumpState() {
    try {
      const response = await fetch('/api/pumpstate');
      if (!response.ok) {
        throw new Error('Erreur HTTP : ' + response.status);
      }
      const data = await response.json();
  
      // Les données sont dans data.pumpstate_data
      const pumpstate = data.pumpstate_data.pumpstate;
      const commandwebon = data.pumpstate_data.commandwebon;
      const commandweb = data.pumpstate_data.commandweb;
  
      const btn1 = document.getElementById('startButton');
      if (pumpstate === 'off') {
        btn1.style.backgroundColor = 'green';
        btn1.disabled = false;
      } else {
        btn1.style.backgroundColor = 'grey';
        btn1.disabled = true;
      }
  
      const btn2 = document.getElementById('stopButton');
      if (pumpstate === 'on') {
        btn2.style.backgroundColor = 'red';
        btn2.disabled = false;
      } else {
        btn2.style.backgroundColor = 'grey';
        btn2.disabled = true;
      }
    
    if (commandwebon) {
        const checkbox = document.getElementById('commandCheckbox');
        checkbox.checked = true;
        document.getElementById('slider').disabled = true;
        document.getElementById('slider').value = commandweb;
        document.getElementById("sliderValue").textContent = commandweb;
        
    } else {
        document.getElementById('slider').disabled = false;
        const checkbox = document.getElementById('commandCheckbox');
        checkbox.checked = false;
    }
  
      return { pumpstate, commandwebon, commandweb };
  
    } catch (error) {
      console.error('Erreur lors de la récupération de l\'API:', error);
    }
  }
  
// Fonction appelée quand on clique sur "Arrêter le système"
function stopSystem() {
    fetch('/api/stoppump', { method: 'POST' })
        .then(response => response.json())
        .then(data => {
            console.log('Pompe arrêtée :', data);
            const btn1 = document.getElementById('startButton');
            btn1.style.backgroundColor = 'green';
            btn1.disabled = false;
            const btn2 = document.getElementById('stopButton');
            btn2.style.backgroundColor = 'grey';
            btn2.disabled = true;
            
        })
        .catch(error => {
            console.error('Erreur d\'arrêt :', error);
            alert("Erreur lors de l'arrêt !");
        });
}



// Fonction appelée à chaque mouvement du slider
function updateSliderValue(value) {
    document.getElementById("sliderValue").textContent = value;
}

// Fonction appelée quand on clique sur "Mettre à jour la commande"
function sendCommand() {
    const value = document.getElementById("slider").value;

    fetch('/api/command', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify({ value: parseInt(value) })
    })
    .then(response => response.json())
    .then(data => {
        console.log("Réponse:", data);
    })
    .catch(error => {
        console.error("Erreur lors de l'envoi de la commande :", error);
        alert("Erreur lors de l'arrêt !", error);
    });
}

function update() {
    fetchPumpState();
    fetchAndUpdate();
}