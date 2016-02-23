var networkId = 0;
$(function () {
	var networkInfos = JSON.parse(PerfRecoder.getIfAdapters());
	var $network = $('#network');
	for (var i = 0; i < networkInfos.length; ++i) {
		var info = networkInfos[i];
		$network.append('<option value="' + info.idx + '">' + info.name + '</option>');
	}

	$('#networkChart').highcharts({
		chart: {
			type: 'spline',
			events: {
				load: function () {
					var series = this.series;
					setInterval(function () {
						var time = (new Date()).getTime();
						if (networkId === 0) {
							series[0].addPoint([time, 0], true, true);
							series[1].addPoint([time, 0], true, true);
							return;
						}

						var speed = JSON.parse(PerfRecoder.getNetTransSpeed());
						speed.recv /= 1024;
						speed.send /= 1024;
						series[0].addPoint([time, speed.recv], true, true);
						series[1].addPoint([time, speed.send], true, true);
						if (series[0].data.length > 50)
							series[0].removePoint(0);
						if (series[1].data.length > 50)
							series[1].removePoint(0);
					}, interval);
				}
			}
		},
		title: {
			text: $('option[value=' + $('#network').val() + ']').text()
		},
		xAxis: {
			type: 'datetime',
			tickPixelInterval: 1500
		},
		yAxis: [{
			title: {
				text: '接收',
				style: {
					color: '#AA4643'
				}
			},
			labels: {
				format: '{value}KB/s',
				style: {
					color: '#AA4643'
				}
			},
			gridLineColor: '#AA4643'
		}, {
			title: {
				text: '发送',
				style: {
					color: '#4572A7'
				}
			},
			labels: {
				format: '{value}KB/s',
				style: {
					color: '#4572A7'
				}
			},
			opposite: true,
			max: 100,
			min: 0
		}],
		tooltip: {
			shared: true,
			valueDecimals: 2,
			dateTimeLabelFormats: {
				millisecond: '%H:%M:%S.%L'
			}
		},
		legend: {
			enabled: true
		},
		series: [{
			name: '接收',
			data: function () {
				var data = [];
				var time = (new Date()).getTime();
				for (var i = -50; i <= 0; ++i)
					data.push({ x: time + i * interval, y: 0 });
				return data;
			}(),
			tooltip: {
				valueSuffix: 'KB/s'
			}
		}, {
			name: '发送',
			data: function () {
				var data = [];
				var time = (new Date()).getTime();
				for (var i = -50; i <= 0; ++i)
					data.push({ x: time + i * interval, y: 0 });
				return data;
			}(),
			yAxis: 1,
			tooltip: {
				valueSuffix: 'KB/s'
			}
		}]
	});
});


function networkMonitoring() {
	networkId = $('#network').val();
	if (networkId == null || networkId == 0 || networkId == '') {
		networkId = 0;
		return;
	}
	networkId = parseInt(networkId, 10);
	PerfRecoder.selectIfAdapter(networkId);

	$('#networkChart').highcharts().setTitle({ text: $('option[value=' + $('#network').val() + ']').text() });
}