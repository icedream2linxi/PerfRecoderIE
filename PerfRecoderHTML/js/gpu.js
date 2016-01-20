$(function () {
	var gpuIds = JSON.parse(PerfRecoder.getPhysicalGPUIds());
	var gpuId = gpuIds[0];

	$('#gpuMem').highcharts({
		chart: {
			type: 'spline',
			events: {
				load: function () {
					var series = this.series;
					setInterval(function () {
						var usage = PerfRecoder.getGPUUsage(gpuId);
						var info = JSON.parse(PerfRecoder.getGPUMemoryInfo(gpuId));
						var time = (new Date()).getTime();
						var usedMemory = info.dedicatedVideoMemory - info.curAvailableDedicatedVideoMemory;
						usedMemory /= 1024;
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
			text: PerfRecoder.getGPUFullName(gpuId)
		},
		xAxis: {
			type: 'datetime',
			tickPixelInterval: 1500
		},
		yAxis: [{
			title: {
				text: '显存使用',
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
				text: 'GPU使用率',
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
			name: '显存使用',
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
			name: 'GPU使用率',
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