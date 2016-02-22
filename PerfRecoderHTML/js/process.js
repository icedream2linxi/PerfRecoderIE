$(function () {
	var processInfos = JSON.parse(PerfRecoder.getAllProcessInfo());
	var $process = $('#process');
	for (var i = 0; i < processInfos.length; ++i) {
		var info = processInfos[i];
		$process.append('<option value="' + info.pid + '">' + info.name + ', ' + info.pid + '</option>');
	}

	$('#processChart').highcharts({
		chart: {
			type: 'spline',
			events: {
				load: function () {
					var series = this.series;
					setInterval(function () {
						var time = (new Date()).getTime();
						if (processId === 0) {
							series[0].addPoint([time, 0], true, true);
							series[1].addPoint([time, 0], true, true);
							return;
						}

						var usage = PerfRecoder.getProcessCPUUsage();
						var info = JSON.parse(PerfRecoder.getProcessMemoryInfo());
						var usedMemory = info.pagefileUsage;
						usedMemory /= 1024 * 1024;
						series[0].addPoint([time, usedMemory], true, true);
						series[1].addPoint([time, usage], true, true);
						if (series[0].data.length > 50)
							series[0].removePoint(0);
						if (series[1].data.length > 50)
							series[1].removePoint(0);
					}, interval);
				}
			}
		},
		title: {
			text: $('option[value=' + $('#process').val() + ']').text()
		},
		xAxis: {
			type: 'datetime',
			tickPixelInterval: 1500
		},
		yAxis: [{
			title: {
				text: '内存使用',
				style: {
					color: '#AA4643'
				}
			},
			labels: {
				formatter: function () {
					return Highcharts.numberFormat(this.value, 2) + 'MB';
				},
				style: {
					color: '#AA4643'
				}
			},
			gridLineColor: '#AA4643'
		}, {
			title: {
				text: 'CPU使用率',
				style: {
					color: '#4572A7'
				}
			},
			labels: {
				format: '{value}%',
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
			name: '内存使用',
			data: function () {
				var data = [];
				var time = (new Date()).getTime();
				for (var i = -50; i <= 0; ++i)
					data.push({ x: time + i * interval, y: 0 });
				return data;
			}(),
			tooltip: {
				valueSuffix: 'MB'
			}
		}, {
			name: 'CPU使用率',
			data: function () {
				var data = [];
				var time = (new Date()).getTime();
				for (var i = -50; i <= 0; ++i)
					data.push({ x: time + i * interval, y: 0 });
				return data;
			}(),
			yAxis: 1,
			tooltip: {
				valueSuffix: '%'
			}
		}]
	});
});